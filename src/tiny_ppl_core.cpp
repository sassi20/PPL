#include "tiny_ppl_core.h"
#include <optional>


SymbolValue::SymbolValue(const std::string& name) : name(name) {}
Value::Type SymbolValue::getType() const {return SYMBOL;}
std::string SymbolValue::toString() const {return "Symbol(" + name + ")";}

NumberValue::NumberValue(double val) : value(val) {}
Value::Type NumberValue::getType() const {return NUMBER;}
std::string NumberValue::toString() const {return std::to_string(value);}

Value::Type Distribution::getType() const {return DISTRIBUTION;}

NormalDistribution::NormalDistribution(double mu, double sigma) : mu(mu), sigma(sigma) {}
std::string NormalDistribution::toString() const {return "Normal(mu=" + std::to_string(mu) + ", sigma=" + std::to_string(sigma) + ")"; }
double NormalDistribution::sample(std::mt19937& rng) const {
    std::normal_distribution<> dist(mu, sigma);
    return dist(rng);
}

BernoulliDistribution::BernoulliDistribution(double p) : p(p) {}
std::string BernoulliDistribution::toString() const { return "Bernoulli(p=" + std::to_string(p) + ")"; }
double BernoulliDistribution::sample(std::mt19937& rng) const {
    std::bernoulli_distribution dist(p);
    return dist(rng) ? 1.0 : 0.0; 
}

Closure::Closure(const std::vector<std::string>& params, 
                const std::vector<std::shared_ptr<Expr>>& body, 
                const std::map<std::string, std::shared_ptr<Value>>& env
                )
                : params(params), body(body), env(env) {}
Value::Type Closure::getType() const {return CLOSURE;}
std::string Closure::toString() const {
    std::string s = "Closure(params: (";
    for (size_t i = 0; i < params.size(); ++i) {
        s += params[i];
        if (i < params.size() - 1) {
            s += ", ";
        }
    }
    s += "), body_forms: " + std::to_string(body.size()) + ", captured_env: " + std::to_string(env.size()) + " vars)";
    return s;
}

//-------------------------------------------------------------------------------------------------------------------------------

SymbolExpr::SymbolExpr(const std::string& name) : name(name) {}
Expr::Type SymbolExpr::getType() const {return SYMBOL;}
std::string SymbolExpr::toString() const {return name;}
 
NumberExpr::NumberExpr(double value) : value(value) {}
Expr::Type NumberExpr::getType() const {return NUMBER;}
std::string NumberExpr::toString() const {return std::to_string(value);}
 
ListExpr::ListExpr(const std::vector<std::shared_ptr<Expr>>& elements) : elements(elements) {}
Expr::Type ListExpr::getType() const { return LIST; }
std::string ListExpr::toString() const {
    std::string s = "(";
    for (size_t i = 0; i < elements.size(); ++i) {
        s += elements[i]->toString();
        if (i + 1 < elements.size()) s += " ";
    }
    s += ")";
    return s;
}

//-------------------------------------------------------------------------------------------------------------------------------

EvInstr::EvInstr(std::shared_ptr<Expr> expr,
    const std::map<std::string, std::shared_ptr<Value>>& env,
    const Address& addr)
: expr(expr), env(env), addr(addr) {}
Instr::Type EvInstr::getType() const {return EV;}

LetKInstr::LetKInstr(const std::vector<std::shared_ptr<Expr>>& binds, int i,
        const std::vector<std::shared_ptr<Expr>>& body,
        const std::map<std::string, std::shared_ptr<Value>>& env,
        const Address& addr)
: binds(binds), i(i), body(body), env(env), addr(addr) {}
Instr::Type LetKInstr::getType() const {return LETK;}

IfKInstr::IfKInstr(std::shared_ptr<Expr> then, std::shared_ptr<Expr> els,
      const std::map<std::string, std::shared_ptr<Value>>& env,
      const Address& addr)
: then(then), els(els), env(env), addr(addr) {}
Instr::Type IfKInstr::getType() const {return IFK;}

Instr::Type DiscardInstr::getType() const {return DISCARD;}

CallKInstr::CallKInstr(int n, const Address& addr) : n(n), addr(addr) {}
Instr::Type CallKInstr::getType() const {return CALLK;}

SampleKInstr::SampleKInstr(const Address& addr) : addr(addr) {}
Instr::Type SampleKInstr::getType() const {return SAMPLEK;}

ObserveKInstr::ObserveKInstr(const Address& addr) : addr(addr) {}
Instr::Type ObserveKInstr::getType() const {return OBSERVEK;}


//-------------------------------------------------------------------------------------------------------------------------------

Message Message::done(std::shared_ptr<Value> val, const MachineState& machine) {
    return Message{DONE, val, Address{}, nullptr, machine};
}
 
Message Message::sample(const Address& addr, std::shared_ptr<Distribution> dist, const MachineState& machine) {
    return Message{SAMPLE, nullptr, addr, dist, machine};
}
 
Message Message::observe(const Address& addr, std::shared_ptr<Distribution> dist, std::shared_ptr<Value> observed_val, const MachineState& machine) {
    return Message{OBSERVE, observed_val, addr, dist, machine};
}

//-------------------------------------------------------------------------------------------------------------------------------

double log_prob(std::shared_ptr<Distribution> d, double x) {
    auto normal_dist = std::dynamic_pointer_cast<NormalDistribution>(d);
    if (normal_dist) {
        double mu = normal_dist->mu;
        double sigma = normal_dist->sigma;
        double z = (x - mu) / sigma;
        double log_normalization = std::log(sigma) + 0.5 * std::log(2 * M_PI);
        return -0.5 * z * z - log_normalization;
    }

    auto bernoulli_dist = std::dynamic_pointer_cast<BernoulliDistribution>(d);
    if (bernoulli_dist) {
        double p = bernoulli_dist->p;
        if (x == 1.0) {
            return std::log(p);
        }
        if (x == 0.0) {
            return std::log(1.0 - p);
        }
        return -std::numeric_limits<double>::infinity();
    }

    std::cerr << "Error: Unsupported distribution type for log_prob" << std::endl;
    return -std::numeric_limits<double>::infinity();
}


//-------------------------------------------------------------------------------------------------------------------------------
// Message Interface

void _push_body(std::vector<std::shared_ptr<Instr>>& controlStack, const std::vector<std::shared_ptr<Expr>>& body_forms,const std::map<std::string, std::shared_ptr<Value>>& env, const Address& address) {
    std::vector<std::shared_ptr<Instr>> seq;
    for (size_t n = 0; n + 1 < body_forms.size(); ++n) {
        Address instr_addr = address;
        instr_addr.push_back("body");
        instr_addr.push_back(std::to_string(n));
        seq.push_back(std::make_shared<EvInstr>(body_forms[n], env, instr_addr));
        seq.push_back(std::make_shared<DiscardInstr>());
    }
    if (!body_forms.empty()) {
        size_t lastN = body_forms.size() - 1;
        Address instr_addr = address;
        instr_addr.push_back("body");
        instr_addr.push_back(std::to_string(lastN));
        seq.push_back(std::make_shared<EvInstr>(body_forms.back(), env, instr_addr));
    }
    for (auto it = seq.rbegin(); it != seq.rend(); ++it) {
        controlStack.push_back(*it);
    }
}

void send(MachineState& m, std::shared_ptr<Value> value) {
    m.stackValue.push_back(value);
}


Message resume(MachineState& m) {
    auto& controlStack = m.controlStack;
    auto& V = m.stackValue;
 
    while (!controlStack.empty()) {
        std::shared_ptr<Instr> instr = controlStack.back();
        controlStack.pop_back();
 
        switch (instr->getType()) {
 
            case Instr::EV: {
                auto ev = std::dynamic_pointer_cast<EvInstr>(instr);
                const std::shared_ptr<Expr>& e = ev->expr;
                const std::map<std::string, std::shared_ptr<Value>>& env = ev->env;
                const Address& addr = ev->addr;
 
                if (e->getType() == Expr::SYMBOL) {
                    auto sym = std::dynamic_pointer_cast<SymbolExpr>(e);
                    auto found = env.find(sym->name);
                    if (found != env.end()) {
                        send(m, found->second);
                    } else if (is_primitive(sym->name)) {
                        send(m, PRIMITIVES[sym->name]);
                    } else {
                        throw std::runtime_error("NameError: " + sym->name);
                    }
                } else if (e->getType() == Expr::NUMBER) {
                    auto num = std::dynamic_pointer_cast<NumberExpr>(e);
                    send(m, std::make_shared<NumberValue>(num->value));
                } else {
                    auto list = std::dynamic_pointer_cast<ListExpr>(e);
                    auto& elems = list->elements;
 
                    bool headIsSymbol = false;
                    std::string headName;
                    if (!elems.empty()) {
                        if (auto headSym = std::dynamic_pointer_cast<SymbolExpr>(elems[0])) {
                            headIsSymbol = true;
                            headName = headSym->name;
                        }
                    }
 
                    if (headIsSymbol && headName == "let") {
                        auto bindsList = std::dynamic_pointer_cast<ListExpr>(elems[1]);
                        std::vector<std::shared_ptr<Expr>> binds = bindsList->elements;
                        std::vector<std::shared_ptr<Expr>> body(elems.begin() + 2, elems.end());
                        if (!binds.empty()) {
                            controlStack.push_back(std::make_shared<LetKInstr>(binds, 0, body, env, addr));
                            Address a2 = addr_append(addr, "let");
                            a2.push_back("0");
                            controlStack.push_back(std::make_shared<EvInstr>(binds[1], env, a2));
                        } else {
                            _push_body(controlStack, body, env, addr);
                        }
 
                    } else if (headIsSymbol && headName == "if") {
                        const auto& test = elems[1];
                        const auto& then_ = elems[2];
                        const auto& els = elems[3];
                        controlStack.push_back(std::make_shared<IfKInstr>(then_, els, env, addr));
                        controlStack.push_back(std::make_shared<EvInstr>(test, env, addr_append(addr, "test")));
 
                    } else if (headIsSymbol && headName == "fn") {
                        auto paramsList = std::dynamic_pointer_cast<ListExpr>(elems[1]);
                        std::vector<std::string> params;
                        for (auto& p : paramsList->elements) {
                            auto psym = std::dynamic_pointer_cast<SymbolExpr>(p);
                            params.push_back(psym->name);
                        }
                        std::vector<std::shared_ptr<Expr>> body(elems.begin() + 2, elems.end());
                        send(m, std::make_shared<Closure>(params, body, env));
 
                    } else if (headIsSymbol && headName == "sample") {
                        controlStack.push_back(std::make_shared<SampleKInstr>(addr));
                        controlStack.push_back(std::make_shared<EvInstr>(elems[1], env, addr_append(addr, "d")));
 
                    } else if (headIsSymbol && headName == "observe") {
                        controlStack.push_back(std::make_shared<ObserveKInstr>(addr));
                        controlStack.push_back(std::make_shared<EvInstr>(elems[2], env, addr_append(addr, "v")));
                        controlStack.push_back(std::make_shared<EvInstr>(elems[1], env, addr_append(addr, "d")));
 
                    } else {
                        int n = static_cast<int>(elems.size()) - 1;
                        controlStack.push_back(std::make_shared<CallKInstr>(n, addr));
                        for (int i = n; i >= 1; --i) {
                            Address ai = addr;
                            ai.push_back(std::to_string(i - 1));
                            controlStack.push_back(std::make_shared<EvInstr>(elems[i], env, ai));
                        }
                        controlStack.push_back(std::make_shared<EvInstr>(elems[0], env, addr_append(addr, "fn")));
                    }
                }
                break;
            }
 
            case Instr::LETK: {
                auto letk = std::dynamic_pointer_cast<LetKInstr>(instr);
                std::map<std::string, std::shared_ptr<Value>> env2 = letk->env;
                auto nameSym = std::dynamic_pointer_cast<SymbolExpr>(letk->binds[2 * letk->i]);
                env2[nameSym->name] = V.back();
                V.pop_back();
 
                if (2 * (letk->i + 1) < static_cast<int>(letk->binds.size())) {
                    controlStack.push_back(std::make_shared<LetKInstr>(letk->binds, letk->i + 1, letk->body, env2, letk->addr));
                    Address a2 = addr_append(letk->addr, "let");
                    a2.push_back(std::to_string(2 * (letk->i + 1)));
                    controlStack.push_back(std::make_shared<EvInstr>(letk->binds[2 * (letk->i + 1) + 1], env2, a2));
                } else {
                    _push_body(controlStack, letk->body, env2, letk->addr);
                }
                break;
            }
 
            case Instr::IFK: {
                auto ifk = std::dynamic_pointer_cast<IfKInstr>(instr);
                auto val = V.back();
                V.pop_back();
                auto numVal = std::dynamic_pointer_cast<NumberValue>(val);
                bool condition;
                if (numVal) {
                    condition = (numVal->value != 0.0);
                } else {
                    condition = (val != nullptr);
                }
                std::shared_ptr<Expr> branchselected;
                std::string tag;
                if (condition) {
                    branchselected = ifk->then;
                    tag = "then";
                } else {
                    branchselected = ifk->els;
                    tag = "else";
                }
                controlStack.push_back(std::make_shared<EvInstr>(branchselected, ifk->env, addr_append(ifk->addr, tag)));
                break;
            }
 
            case Instr::DISCARD: {
                V.pop_back();
                break;
            }
 
            case Instr::CALLK: {
                auto callk = std::dynamic_pointer_cast<CallKInstr>(instr);
                std::vector<std::shared_ptr<Value>> args(callk->n);
                for (int i = callk->n - 1; i >= 0; --i) {
                    args[i] = V.back();
                    V.pop_back();
                }
                auto f = V.back();
                V.pop_back();
 
                if (auto closure = std::dynamic_pointer_cast<Closure>(f)) {
                    std::map<std::string, std::shared_ptr<Value>> new_env = closure->env;
                    for (size_t i = 0; i < closure->params.size(); ++i) {
                        new_env[closure->params[i]] = args[i];
                    }
                    _push_body(controlStack, closure->body, new_env, callk->addr);
                } else if (auto prim = std::dynamic_pointer_cast<PrimitiveValue>(f)) {
                    send(m, prim->fn(args));
                } else {
                    throw std::runtime_error("Error: se intento llamar a un valor que no es funcion ni primitiva");
                }
                break;
            }
 
            case Instr::SAMPLEK: {
                auto samplek = std::dynamic_pointer_cast<SampleKInstr>(instr);
                auto d = std::dynamic_pointer_cast<Distribution>(V.back());
                V.pop_back();
                return Message::sample(samplek->addr, d, m);
            }
 
            case Instr::OBSERVEK: {
                auto observek = std::dynamic_pointer_cast<ObserveKInstr>(instr);
                auto y = V.back();
                V.pop_back();
                auto d = std::dynamic_pointer_cast<Distribution>(V.back());
                V.pop_back();
                return Message::observe(observek->addr, d, y, m);
            }
        }
    }
 
    return Message::done(V.back(), m);
}

