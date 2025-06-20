#include "json_parser.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>

std::map<std::string, std::string> JsonParser::parseObject(const std::string& json) {
    std::map<std::string, std::string> result;
    
    // Simple JSON object parser without complex regex
    size_t pos = 0;
    while (pos < json.length()) {
        // Find key
        size_t keyStart = json.find('"', pos);
        if (keyStart == std::string::npos) break;
        
        size_t keyEnd = json.find('"', keyStart + 1);
        if (keyEnd == std::string::npos) break;
        
        std::string key = json.substr(keyStart + 1, keyEnd - keyStart - 1);
        
        // Find colon
        size_t colonPos = json.find(':', keyEnd + 1);
        if (colonPos == std::string::npos) break;
        
        // Find value
        size_t valueStart = colonPos + 1;
        while (valueStart < json.length() && (json[valueStart] == ' ' || json[valueStart] == '\t' || json[valueStart] == '\n')) {
            valueStart++;
        }
        
        if (valueStart >= json.length()) break;
        
        std::string value;
        if (json[valueStart] == '"') {
            // String value
            size_t valueEnd = json.find('"', valueStart + 1);
            if (valueEnd == std::string::npos) break;
            value = json.substr(valueStart + 1, valueEnd - valueStart - 1);
            pos = valueEnd + 1;
        } else {
            // Number or boolean value
            size_t valueEnd = valueStart;
            while (valueEnd < json.length() && 
                   (std::isalnum(json[valueEnd]) || json[valueEnd] == '.' || json[valueEnd] == '-' || json[valueEnd] == '+')) {
                valueEnd++;
            }
            value = json.substr(valueStart, valueEnd - valueStart);
            pos = valueEnd;
        }
        
        result[key] = unescapeJsonString(value);
        
        // Find next comma or end
        size_t commaPos = json.find(',', pos);
        if (commaPos == std::string::npos) break;
        pos = commaPos + 1;
    }
    
    return result;
}

std::vector<std::map<std::string, std::string>> JsonParser::parseArray(const std::string& json) {
    std::vector<std::map<std::string, std::string>> result;
    
    // Find array content between [ and ]
    size_t start = json.find('[');
    size_t end = json.rfind(']');
    
    if (start == std::string::npos || end == std::string::npos || start >= end) {
        return result;
    }
    
    std::string arrayContent = json.substr(start + 1, end - start - 1);
    std::vector<std::string> objects = splitJsonArray(arrayContent);
    
    for (const auto& objStr : objects) {
        if (!objStr.empty()) {
            result.push_back(parseObject(objStr));
        }
    }
    
    return result;
}

std::string JsonParser::extractString(const std::map<std::string, std::string>& obj, const std::string& key) {
    auto it = obj.find(key);
    return (it != obj.end()) ? it->second : "";
}

int JsonParser::extractInt(const std::map<std::string, std::string>& obj, const std::string& key, int defaultValue) {
    auto it = obj.find(key);
    if (it != obj.end() && !it->second.empty()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool JsonParser::extractBool(const std::map<std::string, std::string>& obj, const std::string& key, bool defaultValue) {
    auto it = obj.find(key);
    if (it != obj.end()) {
        return (it->second == "true");
    }
    return defaultValue;
}

std::vector<std::map<std::string, std::string>> JsonParser::parseGmailMessages(const std::string& json) {
    std::vector<std::map<std::string, std::string>> messages;
    
    // Parse Gmail API response
    auto parsed = parseObject(json);
    std::string messagesJson = extractString(parsed, "messages");
    
    if (!messagesJson.empty()) {
        messages = parseArray(messagesJson);
    }
    
    return messages;
}

std::map<std::string, std::string> JsonParser::parseGmailMessageDetails(const std::string& json) {
    auto parsed = parseObject(json);
    std::map<std::string, std::string> details;
    
    // Extract headers
    std::string headersJson = extractString(parsed, "payload");
    if (!headersJson.empty()) {
        auto headers = parseObject(headersJson);
        std::string headersArray = extractString(headers, "headers");
        
        if (!headersArray.empty()) {
            auto headerList = parseArray(headersArray);
            for (const auto& header : headerList) {
                std::string name = extractString(header, "name");
                std::string value = extractString(header, "value");
                if (!name.empty()) {
                    details[name] = value;
                }
            }
        }
    }
    
    // Extract body
    std::string body = extractString(parsed, "snippet");
    details["body"] = body;
    
    return details;
}

std::vector<std::map<std::string, std::string>> JsonParser::parseWhatsAppMessages(const std::string& json) {
    std::vector<std::map<std::string, std::string>> messages;
    
    // Parse WhatsApp Web API response
    auto parsed = parseObject(json);
    std::string messagesJson = extractString(parsed, "messages");
    
    if (!messagesJson.empty()) {
        messages = parseArray(messagesJson);
    }
    
    return messages;
}

std::vector<std::map<std::string, std::string>> JsonParser::parseTelegramUpdates(const std::string& json) {
    std::vector<std::map<std::string, std::string>> updates;
    
    // Parse Telegram Bot API response
    auto parsed = parseObject(json);
    std::string updatesJson = extractString(parsed, "result");
    
    if (!updatesJson.empty()) {
        updates = parseArray(updatesJson);
    }
    
    return updates;
}

std::map<std::string, std::string> JsonParser::parseTelegramMessage(const std::string& json) {
    auto parsed = parseObject(json);
    std::map<std::string, std::string> message;
    
    // Extract message details
    std::string messageJson = extractString(parsed, "message");
    if (!messageJson.empty()) {
        message = parseObject(messageJson);
    }
    
    return message;
}

std::string JsonParser::unescapeJsonString(const std::string& str) {
    std::string result = str;
    
    // Replace common escape sequences
    std::map<std::string, std::string> replacements = {
        {"\\\"", "\""},
        {"\\\\", "\\"},
        {"\\/", "/"},
        {"\\b", "\b"},
        {"\\f", "\f"},
        {"\\n", "\n"},
        {"\\r", "\r"},
        {"\\t", "\t"}
    };
    
    for (const auto& replacement : replacements) {
        size_t pos = 0;
        while ((pos = result.find(replacement.first, pos)) != std::string::npos) {
            result.replace(pos, replacement.first.length(), replacement.second);
            pos += replacement.second.length();
        }
    }
    
    return result;
}

std::vector<std::string> JsonParser::splitJsonArray(const std::string& json) {
    std::vector<std::string> objects;
    int braceCount = 0;
    std::string currentObject;
    bool inString = false;
    bool escaped = false;
    
    for (char c : json) {
        if (escaped) {
            currentObject += c;
            escaped = false;
            continue;
        }
        
        if (c == '\\') {
            escaped = true;
            currentObject += c;
            continue;
        }
        
        if (c == '"' && !escaped) {
            inString = !inString;
        }
        
        if (!inString) {
            if (c == '{') {
                braceCount++;
            } else if (c == '}') {
                braceCount--;
            }
        }
        
        currentObject += c;
        
        if (braceCount == 0 && c == '}') {
            objects.push_back(currentObject);
            currentObject.clear();
        }
    }
    
    return objects;
} 