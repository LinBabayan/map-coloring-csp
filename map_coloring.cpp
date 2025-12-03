#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <climits>
#include <chrono>
#include <random>
#include <set>
using namespace std;

//  Global Structures
int N;  // number of regions
vector<vector<int>> graphAdj;
vector<vector<int>> domains;
vector<int> assignment;
bool USE_AC3 = true;

// Random Graph Generator 
void generateRandomGraph(int nodes, int edgeProbabilityPercent = 40) {
    N = nodes;
    graphAdj.assign(N, {});
    assignment.assign(N, -1);

    unsigned seed = (unsigned)chrono::system_clock::now().time_since_epoch().count();
    mt19937 rng(seed);
    uniform_int_distribution<int> dist(1, 100);

    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            if (dist(rng) <= edgeProbabilityPercent) {
                graphAdj[i].push_back(j);
                graphAdj[j].push_back(i);
            }
        }
    }
}

// revise -> -1 wiped, 0 nochange, 1 reduced 
int revise(int Xi, int Xj) {
    vector<int> newDi;
    bool reduced = false;

    for (int a : domains[Xi]) {
        bool supported = false;
        for (int b : domains[Xj]) {
            if (a != b) { supported = true; break; } // adjacency constraint
        }
        if (supported) newDi.push_back(a);
        else reduced = true;
    }

    if (newDi.empty()) return -1;          // domain wipe-out
    if (!reduced) return 0;                // unchanged
    domains[Xi].swap(newDi);
    return 1;                              // reduced
}

//  AC-3 
bool AC3() {
    queue<pair<int,int>> q;
    for (int i = 0; i < N; ++i)
        for (int j : graphAdj[i])
            q.push({i, j});

    while (!q.empty()) {
        auto [Xi, Xj] = q.front(); q.pop();
        int status = revise(Xi, Xj);
        if (status == -1) {
            // domain wiped out -> unsatisfiable under current domains
            return false;
        }
        if (status == 1) { // reduced
            // enqueue (Xk, Xi) for all neighbors Xk except Xj
            for (int Xk : graphAdj[Xi]) {
                if (Xk == Xj) continue;
                q.push({Xk, Xi});
            }
        }
        // if status == 0 nothing to do
    }
    return true;
}


// CSP Logic 
bool isConsistent(int var, int value) {
    for (int nb : graphAdj[var])
        if (assignment[nb] == value)
            return false;
    return true;
}

int selectMRV() {
    int best = -1, bestSize = INT_MAX;
    for (int i = 0; i < N; ++i) {
        if (assignment[i] == -1) {
            int sz = (int)domains[i].size();
            if (sz < bestSize) {
                bestSize = sz;
                best = i;
            }
        }
    }
    return best;
}

bool backtrack() {
    bool complete = all_of(assignment.begin(), assignment.end(), [](int x){ return x != -1; });
    if (complete) return true;

    int var = selectMRV();
    if (var == -1) return false; // no variable found but not complete -> failure

    for (int val : domains[var]) {
        if (!isConsistent(var, val)) continue;
        assignment[var] = val;
        if (backtrack()) return true;
        assignment[var] = -1;
    }
    return false;
}

// Solve for minimum colors 
bool solveWithKColors(int k, bool verbose = true) {
    // init domains 0..k-1
    domains.assign(N, {});
    for (int i = 0; i < N; ++i) {
        for (int c = 0; c < k; ++c) domains[i].push_back(c);
    }

    if (verbose) cout << "  Running AC-3 preprocessing... ";
    if (USE_AC3) {
        bool ok = AC3();
        if (verbose) cout << (ok ? "OK\n" : "FAILED (inconsistent)\n");
        if (!ok) return false;
    } else {
        if (verbose) cout << "SKIPPED\n";
    }

    fill(assignment.begin(), assignment.end(), -1);
    bool res = backtrack();
    if (verbose) cout << (res ? "  Backtracking found a solution.\n" : "  Backtracking found NO solution.\n");
    return res;
}

// Graph Visualization 
struct Coord { int x,y; };

vector<Coord> generateCoordinates(int width, int height) {
    vector<Coord> coords(N);
    mt19937 rng((unsigned)chrono::system_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> distX(2, max(2, width - 6));
    uniform_int_distribution<int> distY(0, max(0, height - 1));

    set<pair<int,int>> used;
    for (int i = 0; i < N; ++i) {
        int x, y;
        int attempts = 0;
        do {
            x = distX(rng);
            y = distY(rng);
            attempts++;
            if (attempts > 500) break; // give up if space is tight
        } while (used.count({x,y}) > 0);
        coords[i] = {x,y};
        used.insert({x,y});
    }
    return coords;
}

void printAdjacencyList() {
    cout << "\n===== ADJACENCY LIST =====\n";
    for (int i = 0; i < N; ++i) {
        cout << "Region " << i << ": ";
        for (int nb : graphAdj[i]) cout << nb << " ";
        cout << '\n';
    }
}

void printGraphDiagram() {
    cout << "\n===== GRAPH DIAGRAM =====\n\n";

    const int canvasRows = 20;
    const int canvasCols = 60;
    vector<string> canvas(canvasRows, string(canvasCols, ' '));

    // Generate random positions for nodes (avoiding overlap)
    struct Pos { int r, c; };
    vector<Pos> pos(N);
    set<pair<int,int>> used;
    mt19937 rng((unsigned)chrono::system_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> distR(0, canvasRows-1);
    uniform_int_distribution<int> distC(0, canvasCols-3); // node width

    for (int i = 0; i < N; ++i) {
        int r, c;
        int attempts = 0;
        do {
            r = distR(rng);
            c = distC(rng);
            attempts++;
            if (attempts > 500) break; // give up
        } while (used.count({r,c}) > 0);
        pos[i] = {r,c};
        used.insert({r,c});
    }

    // Draw edges
    auto setCharSafe = [&](int r, int c, char ch){
        if (r<0 || r>=canvasRows || c<0 || c>=canvasCols) return;
        char cur = canvas[r][c];
        if (cur=='(' || cur==')' || (cur>='0' && cur<='9')) return;
        canvas[r][c] = ch;
    };

    for (int u = 0; u < N; ++u) {
        for (int v : graphAdj[u]) {
            if (v <= u) continue; // each edge once
            int r1 = pos[u].r, c1 = pos[u].c+1; // center of node
            int r2 = pos[v].r, c2 = pos[v].c+1;
            int cr = r1, cc = c1;
            while (cr != r2 || cc != c2) {
                int dr = (r2>cr)?1:(r2<cr)?-1:0;
                int dc = (c2>cc)?1:(c2<cc)?-1:0;
                if (dr==0 && dc!=0) setCharSafe(cr,cc,'-');
                else if (dc==0 && dr!=0) setCharSafe(cr,cc,'|');
                else if (dr==dc) setCharSafe(cr,cc,'\\');
                else setCharSafe(cr,cc,'/');
                if (cr!=r2) cr+=dr;
                if (cc!=c2) cc+=dc;
            }
        }
    }

    // Draw nodes
    for (int i=0;i<N;i++) {
        string s="("+to_string(i)+")";
        int r=pos[i].r, c=pos[i].c;
        for (int j=0;j<s.size();j++) {
            if (c+j<canvasCols) canvas[r][c+j]=s[j];
        }
    }

    // Print canvas
    for (auto &line : canvas) cout << line << '\n';
    cout << '\n';
}


int main() {
    // pick a random N between 6 and 12 (you can change range)
    mt19937 rng((unsigned)chrono::system_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> distNodes(6, 12);
    N = distNodes(rng);

    // generate graph (keeps original generator logic)
    generateRandomGraph(N);

    // print adjacency list + diagram
    printAdjacencyList();
    printGraphDiagram();

    // run minimal color search
    cout << "\n===== SOLVING (trying k = 1..N) =====\n";
    int foundK = -1;
    for (int k = 1; k <= N; ++k) {
        cout << "Trying k = " << k << " ...\n";
        bool ok = solveWithKColors(k, /*verbose=*/true);
        if (ok) { foundK = k; break; }
    }

    if (foundK != -1) {
        cout << "\nSolution found with " << foundK << " colors:\n";
        for (int i = 0; i < N; ++i)
            cout << "Region " << i << " -> Color " << assignment[i] << "\n";
    } else {
        cout << "\nNo valid coloring found for k in [1.." << N << "].\n";
    }

    return 0;
}

