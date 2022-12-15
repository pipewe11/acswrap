//
// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4 
//
// MIT License
// 
// Copyright (c) 2022 pipewe11
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <unistd.h>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <regex>
#include <ctime>

#include <boost/format.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

// https://github.com/yhirose/cpp-httplib
#include "httplib.h"

// https://github.com/nlohmann/json
#include "json.hpp"



using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::pair;
using std::to_string;

using json = nlohmann::ordered_json;

string get_mod_cfg_name(string path)
{
    string dir, name;
    std::smatch match;

    if (std::regex_match(path, match, std::regex("^(.*/)(.*)"))) {
        dir = match[1].str();
        name = match[2].str();
    } else {
        dir = "./";
        name = path;
    }
    return dir + "_" + name;
}

void load_textfile(string path, string &text)
{
    std::ifstream ifs(path);
    std::stringstream buf;
    buf << ifs.rdbuf();
    text = buf.str();
}

std::time_t get_write_time(string path)
{
    std::time_t write_time;
    try {
        write_time = boost::filesystem::last_write_time(path);
    } catch (...) {
        write_time = 0;
    }
    return write_time;
}

int main(int argc, char **argv)
{
    string line;
    std::thread *th;
    std::smatch match;

    int ac_detail_port;
    string ac_server_name;
    string ac_server_exe;
    string ac_server_cfg;
    string ac_server_entry;
    int ac_nice;
    string ac_desc;

    string ac_mod_server_cfg;
    string desc;

    int acs_pid = 0;

    namespace po = boost::program_options;
    po::options_description opt;
    opt.add_options()
        //("help,h",   "Show help")
        ("port,p",   po::value<int>(),                   "Port number")
        ("server,s", po::value<std::string>(),           "acServer")
        ("config,c", po::value<std::string>(),           "Config file")
        ("entry,e",  po::value<std::string>(),           "Entry list file")
        ("desc,d",   po::value<std::string>(),           "Description file")
        ("nice,n",   po::value<int>()->default_value(0), "Nice");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, opt), vm);
    } catch(const po::error_with_option_name& e) {
        cout << e.what() << "\n";
    }

    po::notify(vm);
    cout << "port:   " << vm["port"].as<int>() << "\n";
    cout << "server: " << vm["server"].as<string>() << "\n";
    cout << "config: " << vm["config"].as<string>() << "\n";
    cout << "entry:  " << vm["entry"].as<string>() << "\n";
    cout << "desc:   " << vm["desc"].as<string>() << "\n";
    cout << "nice:   " << vm["nice"].as<int>() << "\n";

    ac_detail_port  = vm["port"].as<int>();
    ac_server_exe   = vm["server"].as<string>();
    ac_server_cfg   = vm["config"].as<string>();
    ac_server_entry = vm["entry"].as<string>();
    ac_desc         = vm["desc"].as<string>();
    ac_nice         = vm["nice"].as<int>();

    load_textfile(ac_desc, desc);
    std::time_t desc_write_time = get_write_time(ac_desc);

    //cout << get_mod_cfg_name(ac_server_cfg) << "\n";
    ac_mod_server_cfg = get_mod_cfg_name(ac_server_cfg);

    string section;

    int ac_http_port          = 8081;
    string weather            = "3_clear";
    int tyre_wear             = 100;
    int tyre_grip             = 100;
    int temp_ambient          = 0;
    int temp_road             = 0;
    int frequency             = 15;
    int abs_state             = 1;
    int allowed_tyres_out     = 3;
    int autoclutch_allowed    = 0;
    int damage_multiplier     = 0;
    int force_virtual_mirror  = 1;
    int fuel_rate             = 100;
    int stability_allowed     = 0;
    int tc_state              = 1;
    int tyre_blankets_allowed = 1;

    auto get_int_value = [&](string key, int &var) {
        if (std::regex_match(line, match, std::regex(key))) {
            var = atoi(match[1].str().c_str());
        }
    };

    auto get_str_value = [&](string key, string &var) {
        if (std::regex_match(line, match, std::regex(key))) {
            var = match[1].str();
        }
    };

    auto load_config = [&]() {
        std::ifstream ifs(ac_server_cfg);
        std::ofstream ofs(ac_mod_server_cfg);

        while (getline(ifs, line)) {
            line = std::regex_replace(line, std::regex("\\r"), "");
            if (std::regex_match(line, match, std::regex("^\\[(.+)\\]\\s*"))) {
                section = match[1].str();
            }
            if (section == "SERVER") {
                if (std::regex_match(line, match, std::regex("^NAME=(.*)"))) {
                    ac_server_name = match[1].str();
                    ofs << "NAME=" << ac_server_name << " ℹ" << to_string(ac_detail_port) << "\r\n";
                    continue;
                }

                get_int_value("^HTTP_PORT=\\s*(\\d+)\\s*", ac_http_port);
                get_int_value("^TYRE_WEAR_RATE=\\s*(\\d+)\\s*", tyre_wear);
                get_int_value("^CLIENT_SEND_INTERVAL_HZ=\\s*(\\d+)\\s*", frequency);

                get_int_value("^ABS_ALLOWED=\\s*(\\d+)\\s*",           abs_state);
                get_int_value("^ALLOWED_TYRES_OUT=\\s*(\\d+)\\s*",     allowed_tyres_out);
                get_int_value("^AUTOCLUTCH_ALLOWED=\\s*(\\d+)\\s*",    autoclutch_allowed);
                get_int_value("^DAMAGE_MULTIPLIER=\\s*(\\d+)\\s*",     damage_multiplier);
                get_int_value("^FORCE_VIRTUAL_MIRROR=\\s*(\\d+)\\s*",  force_virtual_mirror);
                get_int_value("^FUEL_RATE=\\s*(\\d+)\\s*",             fuel_rate);
                get_int_value("^STABILITY_ALLOWED=\\s*(\\d+)\\s*",     stability_allowed);
                get_int_value("^TC_ALLOWED=\\s*(\\d+)\\s*",            tc_state);
                get_int_value("^TYRE_BLANKETS_ALLOWED=\\s*(\\d+)\\s*", tyre_blankets_allowed);
            }
            if (section == "WEATHER_0") {
                get_str_value("^GRAPHICS=\\s*(\\d+)\\s*",                 weather);
                get_int_value("^BASE_TEMPERATURE_AMBIENT=\\s*(\\d+)\\s*", temp_ambient);
                get_int_value("^BASE_TEMPERATURE_ROAD=\\s*(\\d+)\\s*",    temp_road);
            }
            if (section == "DYNAMIC_TRACK") {
                get_int_value("^SESSION_START=\\s*(\\d+)\\s*", tyre_grip);
            }
            //ofs << line << endl;
            ofs << line << "\r\n";
        }

        ofs.close();
        ifs.close();
    };

#if 1
    auto ac = [&]() {
        using namespace boost::process;

        string cmd = (boost::format("%s -c  %s -e %s") % ac_server_exe % ac_mod_server_cfg % ac_server_entry).str();

        while (1) {
            load_config();
            ipstream pipe;
            string line;
            child acs(cmd, std_out > pipe);
            acs_pid = acs.id();
            cout << "acServer started" << "\n";
            while (getline(pipe, line)){
                if (line.length() > 0 && line[0] == '{')                continue;
                if (line.length() > 5 && line.substr(0, 5) == "PAGE:")  continue;
                if (line.length() > 6 && line.substr(0, 6) == "Serve ") continue;
                if (line.length() > 3 && line.substr(0, 3) == "REQ")    continue;
                cout << line << "\n";
            }
            acs.wait();
            cout << "acServer stopped" << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(5)); 
        }
        httplib::Client cli("http://127.0.0.1:" + to_string(ac_detail_port));
        cli.Get("/stop");
    };
    th = new std::thread(ac);
#endif

    if (ac_nice != 0) {
        nice(ac_nice);
    }

#if 1
    httplib::Server svr;

    svr.Get("/api/details", [&](const httplib::Request &req, httplib::Response &res) {
        {}
        //cout << req.path << "\n";
        json detail;

        //cout << ac_http_port << "\n";
        httplib::Client cli("http://127.0.0.1:" + to_string(ac_http_port));
        json info, players;
        if (auto r = cli.Get("/INFO")) {
            if (r->status == 200) {
                 info = json::parse(r->body);
            }
        } else {
            res.set_content("error", "text/plain");
            res.status = 503;
            return;
        }
        if (auto r = cli.Get("/JSON\%7C")) {
            if (r->status == 200) {
                 players = json::parse(r->body);
            }
        } else {
            res.set_content("error", "text/plain");
            res.status = 503;
            return;
        }

        json assists;
        assists["absState"]            = abs_state;                                 // ABS_ALLOWED
        assists["allowedTyresOut"]     = allowed_tyres_out;                         // ALLOWED_TYRES_OUT
        assists["autoclutchAllowed"]   = autoclutch_allowed == 1 ? true : false;    // AUTOCLUTCH_ALLOWED
        assists["damageMultiplier"]    = damage_multiplier;                         // DAMAGE_MULTIPLIER
        assists["forceVirtualMirror"]  = force_virtual_mirror == 1 ? true : false;  // FORCE_VIRTUAL_MIRROR
        assists["fuelRate"]            = fuel_rate;                                 // FUEL_RATE
        assists["stabilityAllowed"]    = stability_allowed == 1 ? true : false;     // STABILITY_ALLOWED
        assists["tcState"]             = tc_state;                                  // TC_ALLOWED
        assists["tyreBlanketsAllowed"] = tyre_blankets_allowed == 1 ? true : false; // TYRE_BLANKETS_ALLOWED
        assists["tyreWearRate"]        = tyre_wear;                                 // TYRE_WEAR_RATE
    
        json content;
    
        for (auto car : info["cars"]) {
            content["cars"][car] = {{"url", ""}};
        }
        content["track"] = {{"url", ""}};
        content["password"] = false;
                
        detail = info;
    
        detail["name"] = ac_server_name;
        int session_idx = 0;
        for (int s : detail["sessiontypes"]) {
            if (s <= 2) {
                detail["durations"][session_idx] = int(detail["durations"][session_idx]) * 60 ;
            }
            if (detail["session"] == session_idx) {
                detail["session"] = s;
            }
            session_idx += 1;
        }
        //// test
        //detail["session"] = 2;

        detail["players"] = players;

        std::time_t write_time = get_write_time(ac_desc);
        if (write_time > desc_write_time) {
            load_textfile(ac_desc, desc);
            std::cout << (boost::format("\"%s\" reloaded") % ac_desc).str() << "\n";
            desc_write_time = write_time;
        }
        detail["description"] = desc;

        detail["ambientTemperature"] = temp_ambient;
        detail["roadTemperature"]    = temp_road + temp_ambient;
        detail["windDirection"]      = 0;
        detail["windSpeed"]          = 0;
        detail["currentWeatherId"]   = weather;
        detail["grip"]               = tyre_grip;
        detail["gripTransfer"]       = 0;
        detail["assists"]            = assists;
        detail["maxContactsPerKm"]   = -1;
        detail["city"]               = "";
        detail["passwordChecksum"]   = {"", ""};
        detail["wrappedPort"]        = ac_detail_port;
        detail["content"]            = content;
        detail["frequency"]          = frequency;
        detail["until"]              = std::time(nullptr) + 300;

        res.set_content(detail.dump(), "text/plain");
    });

    svr.Get("/stop", [&](const httplib::Request &req, httplib::Response &res) {
        if (req.remote_addr == "127.0.0.1") {
            kill(acs_pid, SIGTERM);
            res.set_content("ok", "text/html");
            cout << "acsup stopped" << endl;
            svr.stop();
        } else {
            res.set_content("error", "text/html");
        }
    });

    svr.Get("/restart", [&](const httplib::Request &req, httplib::Response &res) {
        if (req.remote_addr == "127.0.0.1") {
            res.set_content("ok", "text/html");
            kill(acs_pid, SIGTERM);
        } else {
            res.set_content("error", "text/html");
        }
    });

    //svr.set_error_handler([](const auto& req, auto& res) {
    //    cout << req.path << endl;
    //    res.set_content("error", "text/html");
    //});

    svr.listen("0.0.0.0", ac_detail_port);
    th->join();
#endif

    cout << "ok" << endl;
}

