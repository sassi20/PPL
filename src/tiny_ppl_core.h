#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <memory>      //std::shared_ptr, std::make_shared
#include <random>      //std::mt19937
#include <limits>      //std::numeric_limits
#include <functional>  //std::function
#include <set>         //std::map<Address, double>
#include <cmath>       //std::log, std::exp, std::sqrt, M_PI
#include <algorithm>   //std::max_element
#include <iomanip>     //std::fixed and std::setprecision
#include <stdexcept>   //std::runtime_error
#include <tuple>       //std::tuple
#include <optional>    //std::optional


using Address = std::vector<std::string>;

inline Address addr_append(const Address& addr, const std::string& token) {
    Address result = addr;
    result.push_back(token);
    return result;
}

//-------------------------------------------------------------------------------------------------------------------------------

class Value {
public:
    enum Type {
        SYMBOL, NUMBER, DISTRIBUTION, CLOSURE, PRIMITIVE
    };
    virtual ~Value() = default;
    virtual Type getType() const = 0;
    virtual std::string toString() const = 0;
};

class SymbolValue : public Value {
public:
        std::string name;
        SymbolValue(const std::string& name);
        Type getType() const override;
        std::string toString() const override;
};

class NumberValue : public Value {
public:
    double value;
    NumberValue(double val);
    Type getType() const override; 
    std::string toString() const override;
};


class Distribution : public Value {
public:
    Type getType() const override; 
    virtual double sample(std::mt19937& rng) const = 0;
};


class NormalDistribution : public Distribution {
public:
    double mu;
    double sigma;
    NormalDistribution(double mu, double sigma);
    std::string toString() const override;
    double sample(std::mt19937& rng) const override;
};


class BernoulliDistribution : public Distribution {
public:
    double p;
    BernoulliDistribution(double p);
    std::string toString() const override;
    double sample(std::mt19937& rng) const override;
};

// Clousure and PrimitiveValue are defined later in the code, after the Expr classes, because they need to have access to Expr and Closure.

//-------------------------------------------------------------------------------------------------------------------------------


class Expr {
public:
    enum Type { SYMBOL, NUMBER, LIST };
    virtual ~Expr() = default;
    virtual Type getType() const = 0;
    virtual std::string toString() const = 0;
};
     
class SymbolExpr : public Expr {
public:
    std::string name;
    SymbolExpr(const std::string& name);
    Type getType() const override;
    std::string toString() const override;
};
     
class NumberExpr : public Expr {
public:
    double value;
    NumberExpr(double value);
    Type getType() const override;
    std::string toString() const override;
};
     
class ListExpr : public Expr {
public:
    std::vector<std::shared_ptr<Expr>> elements;
    ListExpr(const std::vector<std::shared_ptr<Expr>>& elements);
    Type getType() const override;
    std::string toString() const override;
};
    

//--------------------------------------------------------------------------------------------------------------------------------------

// Value subclasses which need to have access to Expr/Closure
class Closure : public Value {
    public:
        std::vector<std::string> params;
        std::vector<std::shared_ptr<Expr>> body;  
        std::map<std::string, std::shared_ptr<Value>> env; 
        Closure(const std::vector<std::string>& params, 
                const std::vector<std::shared_ptr<Expr>>& body,
                const std::map<std::string, std::shared_ptr<Value>>& env);
        
        Type getType() const override;
        std::string toString() const override;
    };
    
    class PrimitiveValue : public Value {
    public:
        std::string name;
        std::function<std::shared_ptr<Value>(const std::vector<std::shared_ptr<Value>>&)> fn;
        PrimitiveValue(const std::string& name,
        std::function<std::shared_ptr<Value>(const std::vector<std::shared_ptr<Value>>&)> fn);
        Type getType() const override;
        std::string toString() const override;
    };
                   
    extern std::map<std::string, std::shared_ptr<PrimitiveValue>> PRIMITIVES;
    bool is_primitive(const std::string& name);

//------------------------------------------------------------------------------------------------------------------------------------

class Instr {
public:
    enum Type { EV, LETK, IFK, DISCARD, CALLK, SAMPLEK, OBSERVEK };
    virtual ~Instr() = default;
    virtual Type getType() const = 0;
};
         

class EvInstr : public Instr {          // ('ev', expr, env, addr)
public:
    std::shared_ptr<Expr> expr;
    std::map<std::string, std::shared_ptr<Value>> env;
    Address addr;
    EvInstr(std::shared_ptr<Expr> expr,
    const std::map<std::string, std::shared_ptr<Value>>& env,
    const Address& addr);
    Type getType() const override;
};
         

class LetKInstr : public Instr {       // ('letk', binds, i, body, env, addr)
public:
    std::vector<std::shared_ptr<Expr>> binds;
    int i;
    std::vector<std::shared_ptr<Expr>> body;
    std::map<std::string, std::shared_ptr<Value>> env;
    Address addr;
    LetKInstr(const std::vector<std::shared_ptr<Expr>>& binds, int i,
        const std::vector<std::shared_ptr<Expr>>& body,
        const std::map<std::string, std::shared_ptr<Value>>& env,
        const Address& addr);
    Type getType() const override;
};
         

class IfKInstr : public Instr {        // ('ifk', then, els, env, addr)
public:
    std::shared_ptr<Expr> then;
    std::shared_ptr<Expr> els;
    std::map<std::string, std::shared_ptr<Value>> env;
    Address addr;
    IfKInstr(std::shared_ptr<Expr> then, std::shared_ptr<Expr> els,
            const std::map<std::string, std::shared_ptr<Value>>& env,
            const Address& addr);
    Type getType() const override;
};
         

class DiscardInstr : public Instr {    // ('discard',)
public:
    Type getType() const override;
};
         

class CallKInstr : public Instr {      // ('callk', n, addr)
public:
    int n;
    Address addr;
    CallKInstr(int n, const Address& addr);
    Type getType() const override;
};
         

class SampleKInstr : public Instr {    // ('samplek', addr)
public:
    Address addr;
    SampleKInstr(const Address& addr);
    Type getType() const override;
};
         

class ObserveKInstr : public Instr {    // ('observek', addr)
public:
    Address addr;
    ObserveKInstr(const Address& addr);
    Type getType() const override;
};

//-------------------------------------------------------------------------------------------------------------------------------

class MachineState {
public:
std::vector<std::shared_ptr<Instr>> controlStack;
    std::vector<std::shared_ptr<Value>> stackValue;
    std::map<std::string, std::shared_ptr<Value>> environment;
    std::mt19937 randomNumber;
    double log_w;

    MachineState(const std::vector<std::shared_ptr<Instr>>& controlStackinit,
      const std::vector<std::shared_ptr<Value>>& stackValueinit, 
      const std::map<std::string, std::shared_ptr<Value>>& environmentinit, 
      std::mt19937 randomNumberinit, double log_w_init);

    MachineState(const std::vector<std::shared_ptr<Instr>>& controlStackinit, std::mt19937 randomNumberinit);

    MachineState fork(std::mt19937* new_rng = nullptr) const;
};


MachineState initMachine(const std::string& program_str, std::mt19937 randomNumber);

//-------------------------------------------------------------------------------------------------------------------------------

struct Message {
    enum Tag { DONE, SAMPLE, OBSERVE };
    Tag tag;
    std::shared_ptr<Value> value;
    Address addr;  
    std::shared_ptr<Distribution> dist;
    MachineState machine;

    static Message done(std::shared_ptr<Value> val, const MachineState& machine);
    static Message sample(const Address& addr, std::shared_ptr<Distribution> dist, const MachineState& machine);
    static Message observe(const Address& addr, std::shared_ptr<Distribution> dist, std::shared_ptr<Value> observed_val, const MachineState& machine);
};


//-------------------------------------------------------------------------------------------------------------------------------


double log_prob(std::shared_ptr<Distribution> d, double x);
    
void send(MachineState& m, std::shared_ptr<Value> value);
    
void _push_body(std::vector<std::shared_ptr<Instr>>& controlStack, const std::vector<std::shared_ptr<Expr>>& body_forms, const Address& current_addr_prefix);

Message resume(MachineState& m);


std::vector<std::shared_ptr<Expr>> parse(const std::string& program);

std::pair<std::shared_ptr<Value>, double> run_lw(const std::string& program, std::mt19937& rng);

std::vector<double> softmax(const std::vector<double>& log_w);

std::pair<std::vector<double>, std::vector<double>> likelihood_weighting(const std::string& program, std::mt19937& rng, int N);

Message advance(MachineState& m);

std::vector<double> run_smc(const std::string& program, std::vector<std::mt19937>& rngs, int N);

double mh_log_alpha(const std::map<Address, double>& current_trace, const std::map<Address, double>& proposed_trace, const std::map<Address, double>& current_sample_log, const std::map<Address, double>& proposed_sample_log, const std::map<Address, double>& current_observed_log, const std::map<Address, double>& proposed_observed_log,const Address& resampled_address);

std::tuple<std::shared_ptr<Value>, std::map<Address,double>, std::map<Address,double>, std::map<Address,double>> run(const std::string& program, std::mt19937& rng, const std::optional<Address>& x0, const std::map<Address, double>& cache);

std::vector<double> single_site_mh(const std::string& program, std::mt19937& rng, int steps, int warmup );