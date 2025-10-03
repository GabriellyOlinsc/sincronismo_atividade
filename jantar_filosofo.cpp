#include <bits/stdc++.h> 
#include <thread>
#include <mutex>
#include <random>
#include <chrono>
#include <atomic>
using namespace std;
using steady_clock = chrono::steady_clock;

enum PhilosState { PENS, FOME, COME };

string stateToStr(PhilosState s){
    switch(s){
        case PENS: return "PENS";
        case FOME: return "FOME";
        case COME: return "COME";
        default: return "????";
    }
}

// Thread-safe random generator por thread
int rand_between(int lo, int hi){
    thread_local std::mt19937 rng(
        (unsigned)chrono::high_resolution_clock::now().time_since_epoch().count() 
        ^ std::hash<std::thread::id>()(this_thread::get_id())
    );
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

int main(int argc, char* argv[]){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // parâmetros
    int N;
    int dur_seconds;
    int think_min_ms, think_max_ms, eat_min_ms, eat_max_ms;

    // agora só aceita pela linha de comando
    if(argc == 7){
        N = stoi(argv[1]);
        dur_seconds = stoi(argv[2]);
        think_min_ms = stoi(argv[3]);
        think_max_ms = stoi(argv[4]);
        eat_min_ms = stoi(argv[5]);
        eat_max_ms = stoi(argv[6]);
    } else {
        cerr << "Uso: " << argv[0] 
             << " N duracao_seconds think_min_ms think_max_ms eat_min_ms eat_max_ms\n";
        return 1;
    }

    if(N < 3){
        cerr << "N deve ser >= 3\n";
        return 1;
    }
    if(think_min_ms < 0 || think_max_ms < think_min_ms || 
       eat_min_ms < 0 || eat_max_ms < eat_min_ms){
        cerr << "Faixas de tempo inválidas\n";
        return 1;
    }

    // dados compartilhados
    vector<mutex> forkMutex(N);
    vector<bool> forkInUse(N, false);
    vector<PhilosState> states(N, PENS);
    vector<int> meals(N, 0);

    // acumuladores de tempo por estado
    vector<chrono::milliseconds> timePens(N, chrono::milliseconds::zero());
    vector<chrono::milliseconds> timeFome(N, chrono::milliseconds::zero());
    vector<chrono::milliseconds> timeCome(N, chrono::milliseconds::zero());

    mutex state_mutex;
    atomic<bool> stopFlag(false);

    auto start_time = steady_clock::now();
    auto end_time = start_time + chrono::seconds(dur_seconds);

    // função da thread filósofo
    auto philosopher = [&](int id){
        int left = id;
        int right = (id + 1) % N;

        thread_local std::mt19937 rng(
            (unsigned)chrono::high_resolution_clock::now().time_since_epoch().count() 
            ^ std::hash<std::thread::id>()(this_thread::get_id())
        );
        std::uniform_int_distribution<int> think_dist(think_min_ms, think_max_ms);
        std::uniform_int_distribution<int> eat_dist(eat_min_ms, eat_max_ms);

        auto last_state_change = steady_clock::now();
        PhilosState cur_state = PENS;

        while(!stopFlag.load()){
            // --- PENSAR
            {
                auto now = steady_clock::now();
                lock_guard<mutex> lk(state_mutex);
                auto delta = chrono::duration_cast<chrono::milliseconds>(now - last_state_change);
                if(cur_state == PENS) timePens[id] += delta;
                else if(cur_state == FOME) timeFome[id] += delta;
                else if(cur_state == COME) timeCome[id] += delta;
                states[id] = PENS;
                cur_state = PENS;
                last_state_change = now;
            }
            this_thread::sleep_for(chrono::milliseconds(think_dist(rng)));
            if(stopFlag.load()) break;

            // --- FOME
            {
                auto now = steady_clock::now();
                lock_guard<mutex> lk(state_mutex);
                auto delta = chrono::duration_cast<chrono::milliseconds>(now - last_state_change);
                if(cur_state == PENS) timePens[id] += delta;
                else if(cur_state == FOME) timeFome[id] += delta;
                else if(cur_state == COME) timeCome[id] += delta;
                states[id] = FOME;
                cur_state = FOME;
                last_state_change = now;
            }

            unique_lock<mutex> lock_left(forkMutex[left], std::defer_lock);
            unique_lock<mutex> lock_right(forkMutex[right], std::defer_lock);
            std::lock(lock_left, lock_right);

            {
                auto now = steady_clock::now();
                lock_guard<mutex> lk(state_mutex);
                forkInUse[left] = true;
                forkInUse[right] = true;
                auto delta = chrono::duration_cast<chrono::milliseconds>(now - last_state_change);
                if(cur_state == FOME) timeFome[id] += delta;
                states[id] = COME;
                cur_state = COME;
                last_state_change = now;
            }

            this_thread::sleep_for(chrono::milliseconds(eat_dist(rng)));

            {
                auto now = steady_clock::now();
                lock_guard<mutex> lk(state_mutex);
                auto delta = chrono::duration_cast<chrono::milliseconds>(now - last_state_change);
                if(cur_state == COME) timeCome[id] += delta;
                meals[id] += 1;
                forkInUse[left] = false;
                forkInUse[right] = false;
                states[id] = PENS;
                cur_state = PENS;
                last_state_change = now;
            }
        }

        auto now = steady_clock::now();
        {
            lock_guard<mutex> lk(state_mutex);
            auto delta = chrono::duration_cast<chrono::milliseconds>(now - last_state_change);
            if(cur_state == PENS) timePens[id] += delta;
            else if(cur_state == FOME) timeFome[id] += delta;
            else if(cur_state == COME) timeCome[id] += delta;
        }
    };

    // start N threads
    vector<thread> threads;
    threads.reserve(N);
    for(int i=0;i<N;++i) threads.emplace_back(philosopher, i);

    // impressões periódicas
    while(steady_clock::now() < end_time){
        auto now = steady_clock::now();
        auto next_print = now + chrono::seconds(1);

        vector<bool> forks_snapshot;
        vector<PhilosState> states_snapshot;
        vector<int> meals_snapshot;
        {
            lock_guard<mutex> lk(state_mutex);
            forks_snapshot = forkInUse;
            states_snapshot = states;
            meals_snapshot = meals;
        }

        cout << "\x1B[2J\x1B[H"; 
        cout << "Garfos: ";
        for(int i=0;i<N;++i) cout << "[" << (forks_snapshot[i] ? "X" : "O") << "]";
        cout << "\nEstados: ";
        for(int i=0;i<N;++i){
            cout << "F" << i << ":" << stateToStr(states_snapshot[i]);
            if(i+1 < N) cout << " | ";
        }
        cout << "\nRefeicoes: ";
        for(int i=0;i<N;++i){
            cout << "F" << i << " comeu: " << meals_snapshot[i];
            if(i+1 < N) cout << " | ";
        }
        cout << "\n";
        auto rem = chrono::duration_cast<chrono::seconds>(end_time - steady_clock::now()).count();
        cout << "Tempo restante (s): " << max<long long>(rem,0) << "\n";
        cout << flush;

        while(steady_clock::now() < next_print && steady_clock::now() < end_time)
            this_thread::sleep_for(chrono::milliseconds(50));
    }

    stopFlag.store(true);
    for(auto &t : threads) if(t.joinable()) t.join();

    cout << "\n===== RESUMO FINAL =====\n";
    {
        lock_guard<mutex> lk(state_mutex);
        for(int i=0;i<N;++i){
            cout << "F" << i << " refeicoes: " << meals[i]
                 << " | tempo PENS: " << chrono::duration_cast<chrono::milliseconds>(timePens[i]).count() << "ms"
                 << " | tempo FOME: " << chrono::duration_cast<chrono::milliseconds>(timeFome[i]).count() << "ms"
                 << " | tempo COME: " << chrono::duration_cast<chrono::milliseconds>(timeCome[i]).count() << "ms"
                 << "\n";
        }
    }
    cout << "=========================\n";
    return 0;
}
