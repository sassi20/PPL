#include "tiny_ppl_core.h"


PrimitiveValue::PrimitiveValue(const std::string& name, std::function<std::shared_ptr<Value>(const std::vector<std::shared_ptr<Value>>&)> fn)
                : name(name), fn(fn) {}
Value::Type PrimitiveValue::getType() const {return PRIMITIVE;}
std::string PrimitiveValue::toString() const {return "Primitive(" + name + ")"; }

 
double as_number(const std::shared_ptr<Value>& v, const std::string& primName) {
    auto n = std::dynamic_pointer_cast<NumberValue>(v);
    if (!n) {
        throw std::runtime_error("Error: argumento no numerico para primitiva '" + primName + "'");
    }
    return n->value;
}

std::map<std::string, std::shared_ptr<PrimitiveValue>> buildPrimitives() {
    std::map<std::string, std::shared_ptr<PrimitiveValue>> table;
    auto add = [&](const std::string& name, auto fn) {
        table[name] = std::make_shared<PrimitiveValue>(name, fn);
    };
    add("+", [](const std::vector<std::shared_ptr<Value>>& args) -> std::shared_ptr<Value> {
        double s = 0.0;
        for (auto& a : args) {
            s += as_number(a, "+");
        }   
        return std::make_shared<NumberValue>(s);
    });
    add("-", [](const std::vector<std::shared_ptr<Value>>& args) -> std::shared_ptr<Value> {
        if (args.empty()) {
            return std::make_shared<NumberValue>(0.0);
        }
        double s = as_number(args[0], "-");
        if (args.size() == 1) {
            return std::make_shared<NumberValue>(-s);
        }
        for (size_t i = 1; i < args.size(); ++i) {
            s -= as_number(args[i], "-");
        }
        return std::make_shared<NumberValue>(s);
    });
    add("*", [](const std::vector<std::shared_ptr<Value>>& args) -> std::shared_ptr<Value> {
        double s = 1.0;
        for (auto& a : args) {
            s *= as_number(a, "*");
        }
        return std::make_shared<NumberValue>(s);
    });
    add("/", [](const std::vector<std::shared_ptr<Value>>& args) -> std::shared_ptr<Value> {
        double s = as_number(args[0], "/");
        for (size_t i = 1; i < args.size(); ++i) {
            s /= as_number(args[i], "/");
        }
        return std::make_shared<NumberValue>(s);
    });
    add("<", [](const std::vector<std::shared_ptr<Value>>& args) -> std::shared_ptr<Value> {
        double result;
        if (as_number(args[0], "<") < as_number(args[1], "<")) {
            result = 1.0;
        } else {
            result = 0.0;
        }
        return std::make_shared<NumberValue>(result);
    });
    
    add(">", [](const std::vector<std::shared_ptr<Value>>& args) -> std::shared_ptr<Value> {
        double result;
        if (as_number(args[0], ">") > as_number(args[1], ">")) {
            result = 1.0;
        } else {
            result = 0.0;
        }
        return std::make_shared<NumberValue>(result);
    });
    
    add("<=", [](const std::vector<std::shared_ptr<Value>>& args) -> std::shared_ptr<Value> {
        double result;
        if (as_number(args[0], "<=") <= as_number(args[1], "<=")) {
            result = 1.0;
        } else {
            result = 0.0;
        }
        return std::make_shared<NumberValue>(result);
    });
    
    add(">=", [](const std::vector<std::shared_ptr<Value>>& args) -> std::shared_ptr<Value> {
        double result;
        if (as_number(args[0], ">=") >= as_number(args[1], ">=")) {
            result = 1.0;
        } else {
            result = 0.0;
        }
        return std::make_shared<NumberValue>(result);
    });
    
    add("=", [](const std::vector<std::shared_ptr<Value>>& args) -> std::shared_ptr<Value> {
        double result;
        if (as_number(args[0], "=") == as_number(args[1], "=")) {
            result = 1.0;
        } else {
            result = 0.0;
        }
        return std::make_shared<NumberValue>(result);
    });
    add("normal", [](const std::vector<std::shared_ptr<Value>>& args) -> std::shared_ptr<Value> {
        double mu = as_number(args[0], "normal");
        double sigma = as_number(args[1], "normal");
        return std::make_shared<NormalDistribution>(mu, sigma);
    });
    
    add("bernoulli", [](const std::vector<std::shared_ptr<Value>>& args) -> std::shared_ptr<Value> {
        double p = as_number(args[0], "bernoulli");
        return std::make_shared<BernoulliDistribution>(p);
    });

    return table;
}


std::map<std::string, std::shared_ptr<PrimitiveValue>> PRIMITIVES = buildPrimitives();

bool is_primitive(const std::string& name) {
    return PRIMITIVES.find(name) != PRIMITIVES.end();
}