#include "tiny_ppl_core.h"

std::vector<double> softmax(const std::vector<double>& log_w) {
    double maxLog = *std::max_element(log_w.begin(), log_w.end());
    std::vector<double> weights(log_w.size());
    double sum = 0.0;
    for (size_t i = 0; i < log_w.size(); ++i) {
        weights[i] = std::exp(log_w[i] - maxLog); 
        sum += weights[i];
    }
    for (auto& w : weights) {
        w /= sum;
    }
    return weights;
}

std::pair<std::shared_ptr<Value>, double> run_lw(const std::string& program, std::mt19937& rng) {
    MachineState m = initMachine(program, rng);
    while (true) {
        Message msg = resume(m);

        if (msg.tag == Message::DONE) {
            rng = m.randomNumber;
            return {msg.value, msg.machine.log_w};
        }
        m = msg.machine;

        if (msg.tag == Message::SAMPLE) {
            double sampled = msg.dist->sample(m.randomNumber);
            send(m, std::make_shared<NumberValue>(sampled));
        } else if (msg.tag == Message::OBSERVE) {
            double y = std::dynamic_pointer_cast<NumberValue>(msg.value)->value;
            m.log_w += log_prob(msg.dist, y);
            send(m, msg.value);
        }
    }
}



std::pair<std::vector<double>, std::vector<double>> likelihood_weighting(const std::string& program, std::mt19937& rng, int N) {
    std::vector<double> values(N);
    std::vector<double> log_ws(N);
    for (int i = 0; i < N; ++i) {
        auto [value, log_w] = run_lw(program, rng);
        values[i] = std::dynamic_pointer_cast<NumberValue>(value)->value;
        log_ws[i] = log_w;
    }

    std::vector<double> weights = softmax(log_ws);
    return {values, weights};
}

Message advance(MachineState& machine) {
    Message msg = resume(machine);

    while (msg.tag == Message::SAMPLE) {
        double sampled = msg.dist->sample(machine.randomNumber);
        send(machine, std::make_shared<NumberValue>(sampled));
        msg = resume(machine);
    } 

    return msg;
}

std::vector<double> run_smc(const std::string& program, std::vector<std::mt19937>& rngs, int N) {
    std::vector<MachineState> particles;
    for (int i = 0; i < N; ++i) {
        particles.push_back(initMachine(program, rngs[i]));
    }

    while (true) {
        std::vector<Message> messages;
        for (auto& p : particles) {
            messages.push_back(advance(p));
        }

        bool allDone = true;
        bool allObserve = true;
        for (auto& msg : messages) {
            if (msg.tag != Message::DONE){
                allDone = false;
            }
            if (msg.tag != Message::OBSERVE) {
                allObserve = false;
            }
        }

        if (allDone) {
            std::vector<double> results(N);
            for (int i = 0; i < N; ++i) {
                results[i] = std::dynamic_pointer_cast<NumberValue>(messages[i].value)->value;
            }
            return results;
        }

        if (!allObserve) {
            throw std::runtime_error("Particles reached different breakpoints: SMC needs a shared observe sequence");
        }

        std::vector<double> log_inc(N);
        std::vector<MachineState> paused;
        for (int i = 0; i < N; ++i) {
            double y = std::dynamic_pointer_cast<NumberValue>(messages[i].value)->value;
            double lp = log_prob(messages[i].dist, y);

            MachineState m = messages[i].machine;
            m.log_w += lp;
            send(m, messages[i].value);

            log_inc[i] = lp;
            paused.push_back(m);
        }

        std::vector<double> weights = softmax(log_inc);
        std::discrete_distribution<int> ancestorDist(weights.begin(), weights.end());

        std::vector<MachineState> newParticles;
        for (int j = 0; j < N; ++j) {
            int i = ancestorDist(rngs[0]);
            newParticles.push_back(paused[i].fork(&rngs[j]));
        }
        particles = newParticles;
    }
}

double mh_log_alpha(const std::map<Address, double>& current_trace, const std::map<Address, double>& proposed_trace, const std::map<Address, double>& current_sample_log, const std::map<Address, double>& proposed_sample_log, const std::map<Address, double>& current_observed_log, const std::map<Address, double>& proposed_observed_log,const Address& resampled_address) {
    std::set<Address> forward_sites = {resampled_address};
    for (auto& key_value : proposed_trace){
        if (current_trace.find(key_value.first) == current_trace.end()){
            forward_sites.insert(key_value .first);
        }
    }

    std::set<Address> rev = {resampled_address};
    for (auto& key_value : current_trace){
        if (proposed_trace.find(key_value.first) == proposed_trace.end()){
            rev.insert(key_value.first);
        }
    }

    double num = 0.0;
    for (auto& key_value : proposed_sample_log) {
        if (forward_sites.find(key_value.first) == forward_sites.end()){
            num += key_value.second;
        }
    }

    for (auto& key_value : proposed_observed_log){
        num += key_value.second;
    }

    double den = 0.0;
    for (auto& key_value : current_sample_log) {
        if (rev.find(key_value.first) == rev.end()){
            den += key_value.second;
        }
    }
    for (auto& key_value : current_observed_log){
        den += key_value.second;
    } 
    return (std::log((double)current_trace.size()) - std::log((double)proposed_trace.size())) + (num - den);
}

std::tuple<std::shared_ptr<Value>, typename std::map<Address, double>, typename std::map<Address, double>, typename std::map<Address, double>> run(const std::string& program, std::mt19937& rng, const std::optional<Address>& resample_address, const std::map<Address, double>& cache) {
    MachineState m = initMachine(program, rng);
    std::map<Address, double> X, S, O;

    while (true) {
        Message msg = resume(m);

        if (msg.tag == Message::SAMPLE) {
            m = msg.machine;
            const Address& a = msg.addr;

            if ((resample_address && a == *resample_address) || cache.find(a) == cache.end()) {
                X[a] = msg.dist->sample(m.randomNumber); 
            } else {
                X[a] = cache.at(a); 
            }

            S[a] = log_prob(msg.dist, X[a]);
            send(m, std::make_shared<NumberValue>(X[a]));

        } else if (msg.tag == Message::OBSERVE) {
            m = msg.machine;
            double y = std::dynamic_pointer_cast<NumberValue>(msg.value)->value;
            O[msg.addr] = log_prob(msg.dist, y);
            send(m, msg.value);

        } else {
            rng = m.randomNumber;
            return {msg.value, X, S, O};
        }
    }
}



std::vector<double> single_site_mh(const std::string& program, std::mt19937& rng, int steps, int warmup = 2000) {
    auto [current_value, current_trace_values, current_sample_log, current_observe_log] =run(program, rng, std::nullopt, {});
    std::vector<double> chain;
    std::uniform_real_distribution<double> uniform01(0.0, 1.0);

    for (int i = 0; i < steps + warmup; ++i) {
        std::uniform_int_distribution<size_t> pickIdx(0, current_trace_values.size() - 1);
        auto it = current_trace_values.begin();
        std::advance(it, pickIdx(rng));
        Address random_address = it->first;

        auto [proposed_value, proposed_trace_values, proposed_sample_log, proposed_observe_log] = run(program, rng, random_address, current_trace_values);

        double log_alpha = mh_log_alpha(current_trace_values, proposed_trace_values, current_sample_log, proposed_sample_log, current_observe_log, proposed_observe_log,random_address);

        if (std::log(uniform01(rng)) < log_alpha) {
            current_value = proposed_value;
            current_trace_values = proposed_trace_values;
            current_sample_log = proposed_sample_log;
            current_observe_log = proposed_observe_log;
        }

        if (i >= warmup) {
            chain.push_back(std::dynamic_pointer_cast<NumberValue>(current_value)->value);
        }
    }
    return chain;
}