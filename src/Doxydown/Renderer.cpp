#include <chrono>
#include <inja/inja.hpp>
#include <nlohmann/json.hpp>
#include "Renderer.hpp"
#include "Exception.hpp"
#include "Log.hpp"
#include "TextUtils.hpp"

Doxydown::Renderer::Renderer(const Config& config)
    : config(config),
      env(std::make_unique<inja::Environment>()) {

    env->add_callback("isEmpty", 1, [](inja::Arguments& args) -> bool {
        const auto arg = args.at(0)->get<std::string>();
        return arg.empty();
    });
    env->add_callback("title", 1, [](inja::Arguments& args) {
        const auto arg = args.at(0)->get<std::string>();
        return TextUtils::title(arg);
    });
    env->add_callback("date", 1, [](inja::Arguments& args) -> std::string {
        const auto arg = args.at(0)->get<std::string>();
        return TextUtils::date(arg);
    });
    env->add_callback("stripNamespace", 1, [](inja::Arguments& args) -> std::string {
        const auto arg = args.at(0)->get<std::string>();
        return TextUtils::stripNamespace(arg);
    });
    env->add_callback("countProperty", 3, [](inja::Arguments& args) -> int {
        const auto arr = args.at(0)->get<nlohmann::json>();
        const auto key = args.at(1)->get<std::string>();
        const auto value = args.at(2)->get<std::string>();
        auto count = 0;
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            auto& obj = *it;
            auto val = obj.at(key);
            if (val == value) count++;
        }
        return count;
    });
    env->add_callback("countProperty2", 5, [](inja::Arguments& args) -> int {
        const auto arr = args.at(0)->get<nlohmann::json>();
        const auto key0 = args.at(1)->get<std::string>();
        const auto value0 = args.at(2)->get<std::string>();
        const auto key1 = args.at(3)->get<std::string>();
        const auto value1 = args.at(4)->get<std::string>();
        auto count = 0;
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            auto& obj = *it;
            if (obj.at(key0) == value0 && obj.at(key1) == value1) count++;
        }
        return count;
    });
    env->add_callback("queryProperty", 3, [](inja::Arguments& args) -> nlohmann::json {
        const auto arr = args.at(0)->get<nlohmann::json>();
        const auto key = args.at(1)->get<std::string>();
        const auto value = args.at(2)->get<std::string>();
        auto ret = nlohmann::json::array();
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            auto& obj = *it;
            if (obj.at(key) == value) {
                ret.push_back(obj);
            }
        }
        return ret;
    });
    env->add_callback("queryProperty2", 5, [](inja::Arguments& args) -> nlohmann::json {
        const auto arr = args.at(0)->get<nlohmann::json>();
        const auto key0 = args.at(1)->get<std::string>();
        const auto value0 = args.at(2)->get<std::string>();
        const auto key1 = args.at(3)->get<std::string>();
        const auto value1 = args.at(4)->get<std::string>();
        auto ret = nlohmann::json::array();
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            auto& obj = *it;
            if (obj.at(key0) == value0 && obj.at(key1) == value1) {
                ret.push_back(obj);
            }
        }
        return ret;
    });
    env->add_callback("render", 2, [=](inja::Arguments& args) -> nlohmann::json {
        const auto name = args.at(0)->get<std::string>();
        const auto data = args.at(1)->get<nlohmann::json>();
        return this->render(name, data);
    });
    env->set_trim_blocks(false);
    env->set_lstrip_blocks(false);
}

Doxydown::Renderer::~Renderer() = default;

void Doxydown::Renderer::render(const std::string& name, const std::string& path, const nlohmann::json& data) const {
    const auto it = templates.find(name);
    if (it == templates.end()) {
        throw EXCEPTION("Template {} not found", name);
    }

    const auto absPath = Path::join(config.outputDir, path);
    std::fstream file(absPath, std::ios::out);
    if (!file) {
        throw EXCEPTION("Failed to open file for writing {}", absPath);
    }
    Log::i("Rendering {}", absPath);
    try {
        env->render_to(file, *it->second, data);
    } catch (std::exception& e) {
        throw EXCEPTION("Failed to render template '{}' error {}", name, e.what());
    }
}

std::string Doxydown::Renderer::render(const std::string& name, const nlohmann::json& data) const {
    const auto it = templates.find(name);
    if (it == templates.end()) {
        throw EXCEPTION("Template {} not found", name);
    }

    std::stringstream ss;
    try {
        env->render_to(ss, *it->second, data);
    } catch (std::exception& e) {
        throw EXCEPTION("Failed to render template '{}' error {}", name, e.what());
    }
    return ss.str();
}

void Doxydown::Renderer::addTemplate(const std::string& name, const std::string& src) {
    try {
        inja::Template tmpl = env->parse(src);
        const auto it = templates.insert(std::make_pair(name, std::make_unique<inja::Template>(std::move(tmpl)))).first;
        env->include_template(it->first, *it->second);
    } catch (std::exception& e) {
        throw EXCEPTION("Failed to parse template '{}' error {}", name, e.what());
    }
}
