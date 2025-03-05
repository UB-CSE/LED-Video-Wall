#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <vector>
#include "client.hpp"

std::vector<Client*> parse_config_throws(std::string file);

#endif
