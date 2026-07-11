# include "tiny_ppl_core.h"
# include <iostream>
# include <iomanip> // For std::fixed and std::setprecision
# include <limits>  // For std::numeric_limits
 
int main() {
    std::cout << std::fixed << std::setprecision(5);
 
    // --- Test Normal Distribution and log_prob ---
    std::shared_ptr<NormalDistribution> normal_dist = std::make_shared<NormalDistribution>(0.0, 1.0); // N(0,1)
    std::cout << "\nTesting Normal Distribution: " << normal_dist->toString() << std::endl;
    std::cout << "log_prob(N(0,1), 0.0) = " << log_prob(normal_dist, 0.0) << " (expected approx -0.9189)" << std::endl;
    std::cout << "log_prob(N(0,1), 1.0) = " << log_prob(normal_dist, 1.0) << " (expected approx -1.4189)" << std::endl;
    std::cout << "log_prob(N(0,1), -1.0) = " << log_prob(normal_dist, -1.0) << " (expected approx -1.4189)" << std::endl;
 
    std::shared_ptr<NormalDistribution> normal_dist2 = std::make_shared<NormalDistribution>(5.0, 2.0); // N(5,2)
    std::cout << "\nTesting Normal Distribution: " << normal_dist2->toString() << std::endl;
    std::cout << "log_prob(N(5,2), 5.0) = " << log_prob(normal_dist2, 5.0) << " (expected approx -1.6122)" << std::endl;
    std::cout << "log_prob(N(5,2), 7.0) = " << log_prob(normal_dist2, 7.0) << " (expected approx -2.1122)" << std::endl;
 
    // --- Test Bernoulli Distribution and log_prob ---
    std::shared_ptr<BernoulliDistribution> bernoulli_dist = std::make_shared<BernoulliDistribution>(0.7); // p=0.7
    std::cout << "\nTesting Bernoulli Distribution: " << bernoulli_dist->toString() << std::endl;
    std::cout << "log_prob(B(0.7), 1.0) = " << log_prob(bernoulli_dist, 1.0) << " (expected approx -0.35667)" << std::endl;
    std::cout << "log_prob(B(0.7), 0.0) = " << log_prob(bernoulli_dist, 0.0) << " (expected approx -1.20397)" << std::endl;
    std::cout << "log_prob(B(0.7), 0.5) = " << log_prob(bernoulli_dist, 0.5) << " (expected -inf)" << std::endl;
 
    // --- Test sampling from distributions ---
    std::mt19937 rng(42); // Seed for reproducibility
    std::cout << "\nTesting Sampling:" << std::endl;
    std::cout << "Normal sample (mu=0, sigma=1): " << normal_dist->sample(rng) << std::endl;
    std::cout << "Bernoulli sample (p=0.7): " << bernoulli_dist->sample(rng) << std::endl;
    std::cout << "Bernoulli sample (p=0.7): " << bernoulli_dist->sample(rng) << std::endl;
 
    // --- Test M ---
    // CAMBIO: el controlStack ya no es vector<string>, ahora es vector<shared_ptr<Instr>>.
    // Ademas la variable ya no se llama "initMachine" para no taparse con la funcion initMachine() del header.
    // Helpers minimos para armar el AST a mano (todavia no tenemos el parser -> initMachine real sin implementar)
    auto Sym = [](const std::string& n) -> std::shared_ptr<Expr> { return std::make_shared<SymbolExpr>(n); };
    auto Num = [](double v) -> std::shared_ptr<Expr> { return std::make_shared<NumberExpr>(v); };
    auto Lst = [](std::vector<std::shared_ptr<Expr>> elems) -> std::shared_ptr<Expr> { return std::make_shared<ListExpr>(elems); };
 
    std::vector<std::shared_ptr<Instr>> initial_C;
    std::map<std::string, std::shared_ptr<Value>> empty_env;
    initial_C.push_back(std::make_shared<EvInstr>(Sym("x"), empty_env, Address{"main"}));
    std::mt19937 generator(100); // Seed for reproducibility
    MachineState m0(initial_C, generator);
    std::cout << "\nInitial M C size: " << m0.controlStack.size() << " (esperado: 1)" << std::endl;
 
 
    std::cout << "\n--- Additional log_prob Tests ---" << std::endl;
 
    // --- Normal Distribution Edge Cases ---
    std::shared_ptr<NormalDistribution> normal_dist_tight = std::make_shared<NormalDistribution >(NormalDistribution(5.0, 0.1)); // N(5, 0.1)
    std::cout << "\nTesting Normal (tight) Distribution: " << normal_dist_tight->toString() << std::endl;
    std::cout << "log_prob(N(5,0.1), 5.0) = " << log_prob(normal_dist_tight, 5.0) << " (expected large negative)" << std::endl;
    std::cout << "log_prob(N(5,0.1), 5.1) = " << log_prob(normal_dist_tight, 5.1) << " (expected smaller negative)" << std::endl;
    std::cout << "log_prob(N(5,0.1), 10.0) = " << log_prob(normal_dist_tight, 10.0) << " (expected -infinity or very large negative)" << std::endl;
 
    // --- Bernoulli Distribution Edge Cases ---
    std::shared_ptr<BernoulliDistribution> bernoulli_dist_low_p = std::make_shared<BernoulliDistribution>(BernoulliDistribution(0.001)); // p=0.001
    std::shared_ptr<BernoulliDistribution> bernoulli_dist_high_p = std::make_shared<BernoulliDistribution>(BernoulliDistribution(0.999)); // p=0.999
    std::shared_ptr<BernoulliDistribution> bernoulli_dist_fifty = std::make_shared<BernoulliDistribution>(BernoulliDistribution(0.5)); // p=0.5
 
    std::cout << "\nTesting Bernoulli (low p) Distribution: " << bernoulli_dist_low_p->toString() << std::endl;
    std::cout << "log_prob(B(0.001), 1.0) = " << log_prob(bernoulli_dist_low_p, 1.0) << " (expected large negative)" << std::endl;
    std::cout << "log_prob(B(0.001), 0.0) = " << log_prob(bernoulli_dist_low_p, 0.0) << " (expected small negative)" << std::endl;
 
    std::cout << "\nTesting Bernoulli (high p) Distribution: " << bernoulli_dist_high_p->toString() << std::endl;
    std::cout << "log_prob(B(0.999), 1.0) = " << log_prob(bernoulli_dist_high_p, 1.0) << " (expected small negative)" << std::endl;
    std::cout << "log_prob(B(0.999), 0.0) = " << log_prob(bernoulli_dist_high_p, 0.0) << " (expected large negative)" << std::endl;
 
    std::cout << "\nTesting Bernoulli (p=0.5) Distribution: " << bernoulli_dist_fifty->toString() << std::endl;
    std::cout << "log_prob(B(0.5), 1.0) = " << log_prob(bernoulli_dist_fifty, 1.0) << " (expected approx -0.693)" << std::endl; // ln(0.5)
    std::cout << "log_prob(B(0.5), 0.0) = " << log_prob(bernoulli_dist_fifty, 0.0) << " (expected approx -0.693)" << std::endl;
 
    // --- Invalid value for Bernoulli ---
    std::cout << "\nTesting Bernoulli with invalid value:" << std::endl;
    std::cout << "log_prob(B(0.7), 0.5) = " << log_prob(bernoulli_dist, 0.5) << " (expected -infinity)" << std::endl;
 
    // --- Null distribution ---
    std::cout << "\nTesting log_prob with null distribution (expected -infinity and error):" << std::endl;
    std::shared_ptr<Distribution> null_dist = nullptr;
    std::cout << "log_prob(nullptr, 0.0) = " << log_prob(null_dist, 0.0) << std::endl;
 
 
    // ================== Testing resume() ==================
    std::cout << "\n--- Testing resume() ---" << std::endl;
 
    // Test 1: (+ 1 2) -> deberia dar DONE con NumberValue(3)
    {
        std::shared_ptr<Expr> program = Lst({Sym("+"), Num(1.0), Num(2.0)});
        std::vector<std::shared_ptr<Instr>> C;
        std::map<std::string, std::shared_ptr<Value>> env;
        C.push_back(std::make_shared<EvInstr>(program, env, Address{"main"}));
        MachineState m1(C, std::mt19937(1));
        Message msg = resume(m1);
        std::cout << "\n(+ 1 2) -> tag=" << msg.tag << " valor=" << msg.value->toString()
                  << " (esperado: tag=0 (DONE), 3.000000)" << std::endl;
    }
 
    // Test 2: (if (< 1 2) 10 20) -> deberia dar DONE con 10
    {
        std::shared_ptr<Expr> test = Lst({Sym("<"), Num(1.0), Num(2.0)});
        std::shared_ptr<Expr> program = Lst({Sym("if"), test, Num(10.0), Num(20.0)});
        std::vector<std::shared_ptr<Instr>> C;
        std::map<std::string, std::shared_ptr<Value>> env;
        C.push_back(std::make_shared<EvInstr>(program, env, Address{"main"}));
        MachineState m2(C, std::mt19937(1));
        Message msg = resume(m2);
        std::cout << "\n(if (< 1 2) 10 20) -> tag=" << msg.tag << " valor=" << msg.value->toString()
                  << " (esperado: tag=0 (DONE), 10.000000)" << std::endl;
    }
 
    // Test 3: (let (x 5 y 10) (+ x y)) -> deberia dar DONE con 15
    {
        std::shared_ptr<Expr> binds = Lst({Sym("x"), Num(5.0), Sym("y"), Num(10.0)});
        std::shared_ptr<Expr> body = Lst({Sym("+"), Sym("x"), Sym("y")});
        std::shared_ptr<Expr> program = Lst({Sym("let"), binds, body});
        std::vector<std::shared_ptr<Instr>> C;
        std::map<std::string, std::shared_ptr<Value>> env;
        C.push_back(std::make_shared<EvInstr>(program, env, Address{"main"}));
        MachineState m3(C, std::mt19937(1));
        Message msg = resume(m3);
        std::cout << "\n(let (x 5 y 10) (+ x y)) -> tag=" << msg.tag << " valor=" << msg.value->toString()
                  << " (esperado: tag=0 (DONE), 15.000000)" << std::endl;
    }
 
    // Test 4: ((fn (x) (* x x)) 5) -> deberia dar DONE con 25
    {
        std::shared_ptr<Expr> fn_expr = Lst({Sym("fn"), Lst({Sym("x")}), Lst({Sym("*"), Sym("x"), Sym("x")})});
        std::shared_ptr<Expr> program = Lst({fn_expr, Num(5.0)});
        std::vector<std::shared_ptr<Instr>> C;
        std::map<std::string, std::shared_ptr<Value>> env;
        C.push_back(std::make_shared<EvInstr>(program, env, Address{"main"}));
        MachineState m4(C, std::mt19937(1));
        Message msg = resume(m4);
        std::cout << "\n((fn (x) (* x x)) 5) -> tag=" << msg.tag << " valor=" << msg.value->toString()
                  << " (esperado: tag=0 (DONE), 25.000000)" << std::endl;
    }
 
    // Test 5: (sample d), con d=Normal(0,1) puesta directo en el env
    // (todavia no hay primitiva "normal" para construir distribuciones desde el AST,
    // por eso la distribucion se liga a mano en el entorno)
    {
        std::map<std::string, std::shared_ptr<Value>> env;
        env["d"] = normal_dist; // N(0,1) definida arriba en los tests de distribuciones
        std::shared_ptr<Expr> program = Lst({Sym("sample"), Sym("d")});
        std::vector<std::shared_ptr<Instr>> C;
        C.push_back(std::make_shared<EvInstr>(program, env, Address{"main"}));
        MachineState m5(C, std::mt19937(7));
        Message msg = resume(m5);
        std::cout << "\n(sample d) -> tag=" << msg.tag << " (esperado: tag=1 (SAMPLE))" << std::endl;
 
        if (msg.tag == Message::SAMPLE) {
            MachineState resumed = msg.machine;
            double sampled = msg.dist->sample(resumed.randomNumber);
            send(resumed, std::make_shared<NumberValue>(sampled));
            Message final_msg = resume(resumed);
            std::cout << "  valor sampleado=" << sampled << ", al continuar -> tag=" << final_msg.tag
                       << " valor=" << final_msg.value->toString()
                       << " (esperado: tag=0 (DONE), igual al valor sampleado)" << std::endl;
        }
    }
 
    // Test 6: (observe d 0.5), con d=Normal(0,1) puesta directo en el env
    {
        std::map<std::string, std::shared_ptr<Value>> env;
        env["d"] = normal_dist; // N(0,1)
        std::shared_ptr<Expr> program = Lst({Sym("observe"), Sym("d"), Num(0.5)});
        std::vector<std::shared_ptr<Instr>> C;
        C.push_back(std::make_shared<EvInstr>(program, env, Address{"main"}));
        MachineState m6(C, std::mt19937(1));
        Message msg = resume(m6);
        std::cout << "\n(observe d 0.5) -> tag=" << msg.tag << " (esperado: tag=2 (OBSERVE))" << std::endl;
 
        if (msg.tag == Message::OBSERVE) {
            double y = std::dynamic_pointer_cast<NumberValue>(msg.value)->value;
            double lp = log_prob(msg.dist, y);
            MachineState resumed = msg.machine;
            resumed.log_w += lp;
            send(resumed, msg.value); // "observe" sigue la ejecucion devolviendo el valor observado
            Message final_msg = resume(resumed);
            std::cout << "  log_prob(N(0,1), 0.5) = " << lp << " (esperado aprox -1.0439)"
                       << ", log_w acumulado=" << resumed.log_w
                       << ", al continuar -> tag=" << final_msg.tag << " (esperado: tag=0 (DONE))" << std::endl;
        }
    }


    // ================== Testing parse() ==================
    std::cout << "\n--- Testing parse() ---" << std::endl;

    // Test A: expresion simple con parentesis
    {
        auto forms = parse("(+ 1 2)");
        std::cout << "\nparse(\"(+ 1 2)\") -> cantidad de formas=" << forms.size()
                << " (esperado: 1)" << std::endl;
        std::cout << "  forms[0].toString() = " << forms[0]->toString()
                << " (esperado: (+ 1.000000 2.000000))" << std::endl;
    }

    // Test B: anidamiento con corchetes para bindings (let)
    {
        auto forms = parse("(let [x 3 y 7] (if (< x y) 10 20))");
        std::cout << "\nparse(let/if con corchetes) -> cantidad de formas=" << forms.size()
                << " (esperado: 1)" << std::endl;
        std::cout << "  forms[0].toString() = " << forms[0]->toString() << std::endl;
        // chequeo puntual: el head de la forma tiene que ser el simbolo "let"
        auto list = std::dynamic_pointer_cast<ListExpr>(forms[0]);
        auto head = std::dynamic_pointer_cast<SymbolExpr>(list->elements[0]);
        std::cout << "  head->name = " << head->name << " (esperado: let)" << std::endl;
    }

    // Test C: varias formas de nivel superior (defn + main), como el ejemplo de geom
    {
        auto forms = parse("(defn suma3 [n] (if (< n 1) 0 (+ n (suma3 (- n 1))))) (suma3 3)");
        std::cout << "\nparse(defn + main) -> cantidad de formas=" << forms.size()
                << " (esperado: 2)" << std::endl;
        std::cout << "  forms[0].toString() = " << forms[0]->toString() << std::endl;
        std::cout << "  forms[1].toString() = " << forms[1]->toString()
                << " (esperado: (suma3 3.000000))" << std::endl;
    }

    // Test D: numeros negativos y decimales
    {
        auto forms = parse("(+ -2.5 3.25)");
        auto list = std::dynamic_pointer_cast<ListExpr>(forms[0]);
        auto n1 = std::dynamic_pointer_cast<NumberExpr>(list->elements[1]);
        auto n2 = std::dynamic_pointer_cast<NumberExpr>(list->elements[2]);
        std::cout << "\nparse(\"(+ -2.5 3.25)\") -> n1=" << n1->value << " n2=" << n2->value
                << " (esperado: n1=-2.50000, n2=3.25000)" << std::endl;
    }

    // Test E: parentesis anidados varios niveles
    {
        auto forms = parse("(if (< (+ 1 1) 3) (* 2 2) 0)");
        std::cout << "\nparse(anidado profundo) -> " << forms[0]->toString()
                << " (esperado: (if (< (+ 1.000000 1.000000) 3.000000) (* 2.000000 2.000000) 0.000000))"
                << std::endl;
    }

    // Test F: error esperado, parentesis sin cerrar
    {
        std::cout << "\nprobando parseo invalido \"(+ 1 2\" (deberia tirar excepcion):" << std::endl;
        try {
            auto forms = parse("(+ 1 2");
            std::cout << "  ERROR: no tiro excepcion (esto NO deberia pasar)" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "  ok, exception capturada: " << e.what() << std::endl;
        }
    }
    
    std::cout << std::fixed << std::setprecision(3);

    // ---------------- Testing single_site_mh() ------------------
    
    std::cout << "\n--- Single-site MH (conj) ---" << std::endl;
    std::string conj = "(let [mu (sample (normal 0 1))] (observe (normal mu 1) 2.3) mu)";
    std::mt19937 rng_mh(0);
    std::vector<double> ch = single_site_mh(conj, rng_mh, 60000, 3000);

    double mean = 0.0;
    for (double v : ch) mean += v;
    mean /= ch.size();

    double sq = 0.0;
    for (double v : ch) sq += (v - mean) * (v - mean);
    double stdv = std::sqrt(sq / ch.size());

    std::cout << "conj SSMH mean=" << mean << " std=" << stdv << " (esperado: mean~1.150, std~0.707)" << std::endl;

    // ----------------------- LW vs SMC vs SSMH sobre el mismo modelo -----------------------------------
    std::cout << "\n--- LW vs SMC vs SSMH sobre el mismo modelo ---" << std::endl;

    std::mt19937 rng_lw(2);
    auto [vals, weights] = likelihood_weighting(conj, rng_lw, 100000);
    double lw_mean = 0.0;
    for (size_t i = 0; i < vals.size(); ++i) lw_mean += weights[i] * vals[i];
    std::cout << "LW   mean = " << lw_mean << std::endl;

    std::vector<std::mt19937> rngs_smc;
    for (int i = 0; i < 20000; ++i) rngs_smc.emplace_back(1000 + i);
    std::vector<double> smc = run_smc(conj, rngs_smc, 20000);
    double smc_mean = 0.0;
    for (double v : smc) smc_mean += v;
    smc_mean /= smc.size();
    std::cout << "SMC  mean = " << smc_mean << std::endl;

    std::cout << "SSMH mean = " << mean << "   (los tres deberian rondar 1.150)" << std::endl;

    // ----------------------- Testing closures and recursion -----------------------------------

    std::cout << "\n--- Closures (scope lexico) ---" << std::endl;
    std::string shift = "(let [make-shift (fn [mu] (fn [x] (+ x mu)))  f (make-shift 10)] (f 3))";
    std::mt19937 rng_shift(0);
    auto [shift_val, shift_logw] = run_lw(shift, rng_shift);
    std::cout << "(f 3) = " << std::dynamic_pointer_cast<NumberValue>(shift_val)->value << " (esperado: 13)" << std::endl;


    std::cout << "\n--- Recursion (geom) ---" << std::endl;
    std::string geom = "(defn geom [] (if (sample (bernoulli 0.3)) 0 (+ 1 (geom)))) (geom)";
    std::mt19937 rng_geom(1);
    int trials = 200000;
    double ksum = 0.0;
    for (int i = 0; i < trials; ++i) {
        auto [v, lw] = run_lw(geom, rng_geom);
        ksum += std::dynamic_pointer_cast<NumberValue>(v)->value;
    }
    std::cout << "geom mean = " << (ksum / trials) << "   (esperado (1-p)/p = 2.333)" << std::endl;








    return 0;
}