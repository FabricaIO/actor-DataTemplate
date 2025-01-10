#include "DataTemplate.h"

/// @brief Creates a Data Template object
/// @param ConfigFile The file name to store settings in
DataTemplate::DataTemplate(String ConfigFile) {
	config_path = "/settings/act/" + ConfigFile;
}

/// @brief Starts a Data Template object
/// @return True on success
bool DataTemplate::begin() {
	// Set description
	Description.actionQuantity = 1;
	Description.type = "dataformat";
	Description.name = "Data Template";
	Description.actions = {{"Get Data", 0}};
	bool result = false;
	// Create settings directory if necessary
	if (!checkConfig(config_path)) {
		// Set defaults
		current_config = { .template_start = "", .template_end = "", .template_data = "{name=\"%PARAMETER%\",type=\"%UNIT%\"}%VALUE%%N%" };
		result = saveConfig(config_path, getConfig());
	} else {
		// Load settings
		result = setConfig(Storage::readFile(config_path), false);
	}
	return result;
}

/// @brief Receives an action
/// @param action The action to process (only option is 0 for get data)
/// @param payload Not used
/// @return A plaintext response with the data
std::tuple<bool, String> DataTemplate::receiveAction(int action, String payload) {
	if (action == 0) {
		// Take measurement
		SensorManager::takeMeasurement();
		// Allocate the JSON document
		JsonDocument doc;
		// Deserialize file contents
		DeserializationError error = deserializeJson(doc, SensorManager::getLastMeasurement());
		// Test if parsing succeeds.
		if (error) {
			Logger.print(F("Deserialization failed: "));
			Logger.println(error.f_str());
			return { false, "ERROR" };
		}
		// Build response
		String data = current_config.template_start;
		for (const auto& m : doc["measurements"].as<JsonArray>()) {
			String new_line = current_config.template_data;
			new_line.replace("%PARAMETER%", m["parameter"].as<String>());
			new_line.replace("%UNIT%", m["unit"].as<String>());
			new_line.replace("%VALUE%", m["value"].as<String>());
			data += new_line;
		}
		data += current_config.template_end;
		data.replace("%N%",String('\n'));
		return { false, data };
	}
	return { true, R"({"Response": "FAIL"})" };
}

/// @brief Gets the current config
/// @return A JSON string of the config
String DataTemplate::getConfig() {
	// Allocate the JSON document
	JsonDocument doc;
	// Assign current values
	doc["template_start"] = current_config.template_start;
	doc["template_end"] = current_config.template_end;
	doc["template_data"] = current_config.template_data;

	// Create string to hold output
	String output;
	// Serialize to string
	serializeJson(doc, output);
	return output;
}

/// @brief Sets the configuration for this device
/// @param config A JSON string of the configuration settings
/// @param save If the configuration should be saved to a file
/// @return True on success
bool DataTemplate::setConfig(String config, bool save) {
	// Allocate the JSON document
  	JsonDocument doc;
	// Deserialize file contents
	DeserializationError error = deserializeJson(doc, config);
	// Test if parsing succeeds.
	if (error) {
		Logger.print(F("Deserialization failed: "));
		Logger.println(error.f_str());
		return false;
	}
	// Assign loaded values
	current_config.template_start = doc["template_start"].as<String>();
	current_config.template_end = doc["template_end"].as<String>();
	current_config.template_data = doc["template_data"].as<String>();
	if (save) {
		return saveConfig(config_path, getConfig());
	}
	return true;
}