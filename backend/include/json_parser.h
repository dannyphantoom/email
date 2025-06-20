#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

// Simple JSON parser for API responses
class JsonParser {
public:
    static std::map<std::string, std::string> parseObject(const std::string& json);
    static std::vector<std::map<std::string, std::string>> parseArray(const std::string& json);
    static std::string extractString(const std::map<std::string, std::string>& obj, const std::string& key);
    static int extractInt(const std::map<std::string, std::string>& obj, const std::string& key, int defaultValue = 0);
    static bool extractBool(const std::map<std::string, std::string>& obj, const std::string& key, bool defaultValue = false);
    
    // Gmail-specific parsers
    static std::vector<std::map<std::string, std::string>> parseGmailMessages(const std::string& json);
    static std::map<std::string, std::string> parseGmailMessageDetails(const std::string& json);
    
    // WhatsApp-specific parsers
    static std::vector<std::map<std::string, std::string>> parseWhatsAppMessages(const std::string& json);
    
    // Telegram-specific parsers
    static std::vector<std::map<std::string, std::string>> parseTelegramUpdates(const std::string& json);
    static std::map<std::string, std::string> parseTelegramMessage(const std::string& json);

private:
    static std::string unescapeJsonString(const std::string& str);
    static std::vector<std::string> splitJsonArray(const std::string& json);
};

#endif // JSON_PARSER_H 