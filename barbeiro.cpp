#include <bits/stdc++.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <atomic>
using namespace std;
using steady_clock = chrono::steady_clock;

enum BarberState { DORME, ATENDE };

string barberToStr(BarberState s){
    return (s==DORME) ? "DORME" : "ATENDE";
}

// random helper
int rand_between(int lo, int hi){
    thread_local std::mt19937 rng(
        (unsigned)chrono::high_resolution_clock::now().time_since_epoch().count() 
        ^ hash<thread::id>()(this_thread::get_id())
    );
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

int main(int argc, char* argv[]){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // parâmetros
    int numCadeiras;
    int chegadaMin, chegadaMax;
    int atMin, atMax;
    int duracaoSeg;

    if(argc == 7){
        numCadeiras = stoi(argv[1]);
        chegadaMin  = stoi(argv[2]);
        chegadaMax  = stoi(argv[3]);
        atMin       = stoi(argv[4]);
        atMax       = stoi(argv[5]);
        duracaoSeg  = stoi(argv[6]);
    } else {
        cerr << "Uso: ./barbeiro cadeiras chegada_min chegada_max atend_min atend_max duracao_seg\n";
        return 1;
    }

    // fila e estados
    queue<int> fila;
    int proxClienteId = 0;
    mutex mtx;
    condition_variable cv;

    BarberState barberState = DORME;
    atomic<bool> stopFlag(false);

    // contadores
    int atendidos = 0, desistentes = 0;

    // thread do barbeiro
    auto barbeiro = [&](){
        while(!stopFlag.load()){
            unique_lock<mutex> lk(mtx);
            cv.wait(lk, [&]{ return stopFlag.load() || !fila.empty(); });

            if(stopFlag.load()) break;

            // pega cliente da fila
            int cliente = fila.front(); fila.pop();
            barberState = ATENDE;
            lk.unlock();

            // simula atendimento
            int tempo = rand_between(atMin, atMax);
            this_thread::sleep_for(chrono::milliseconds(tempo));

            lk.lock();
            atendidos++;
            barberState = fila.empty() ? DORME : ATENDE;
            lk.unlock();
        }
    };

    // thread geradora de clientes
    auto gerador = [&](){
        while(!stopFlag.load()){
            int espera = rand_between(chegadaMin, chegadaMax);
            this_thread::sleep_for(chrono::milliseconds(espera));
            if(stopFlag.load()) break;

            lock_guard<mutex> lk(mtx);
            int id = proxClienteId++;
            if((int)fila.size() < numCadeiras){
                fila.push(id);
                cv.notify_one();
            } else {
                desistentes++;
            }
        }
    };

    // inicia threads
    thread tBarbeiro(barbeiro);
    thread tGerador(gerador);

    // loop de monitoramento
    auto start = steady_clock::now();
    auto end   = start + chrono::seconds(duracaoSeg);
    while(steady_clock::now() < end){
        this_thread::sleep_for(chrono::seconds(1));

        lock_guard<mutex> lk(mtx);
        cout << "\x1B[2J\x1B[H"; // limpa tela
        cout << "Barbeiro: " << barberToStr(barberState) << "\n";
        cout << "Fila [" ;
        int used = fila.size();
        for(int i=0;i<numCadeiras;i++){
            cout << (i < used ? "#" : ".");
        }
        cout << "] (" << used << "/" << numCadeiras << ")\n";
        cout << "Atendidos: " << atendidos
             << " | Desistentes: " << desistentes
             << " | Em espera: " << fila.size() << "\n";
    }

    // finalização
    stopFlag.store(true);
    cv.notify_all();
    tGerador.join();
    tBarbeiro.join();

    cout << "\n=== RESUMO FINAL ===\n";
    cout << "Clientes atendidos: " << atendidos << "\n";
    cout << "Clientes desistentes: " << desistentes << "\n";
    cout << "Clientes restantes em espera: " << fila.size() << "\n";
    cout << "====================\n";

    return 0;
}
