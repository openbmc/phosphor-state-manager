#include "utility.hpp"

#include <experimental/filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace utility;

constexpr auto gpioDefsFile = "/etc/default/obmc/gpio/gpio_defs.json";

int main (void)
{
    std::ifstream gpioDefsStream(gpioDefsFile);

    if (gpioDefsStream.is_open())
    {
        auto data = json::parse(gpioDefsStream, nullptr, false);

        if (!data.is_discarded())
        {
            auto gpios = data["gpio_configs"]["power_config"]["power_up_outs"];
            if (gpios.size() > 0)
            {
                auto defs = data["gpio_definitions"];

                for (const auto& gpio : gpios)
                {
                    for (const auto& def : defs)
                    {
                        if (def["name"] == gpio["name"])
                        {
                            std::string pin = def["pin"];
                            std::stringstream ss(gpio["polarity"].dump());
                            bool activeLow;
                            ss >> std::boolalpha >> activeLow;
                        
                            if (!gpioSetValue(pin.c_str(), !activeLow, false))
                            {
                                return 1;
                            }

                            break;
                        }
                    }
                }
            }
            else
            {
                fprintf(stderr, "Could not find power_up_outs defs in %s\n",
                        gpioDefsFile);
                return 1;
            }
        }
        else
        {
            fprintf(stderr, "Error parsing file %s\n", gpioDefsFile);
            return 1;
        }
    }
    else
    {
        fprintf(stderr, "Error opening file %s\n", gpioDefsFile);
        return 1;
    }

    return 0;
}
