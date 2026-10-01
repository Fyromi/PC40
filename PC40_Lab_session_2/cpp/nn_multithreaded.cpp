// Small fully-connected neural network for MNIST:
//     784 inputs -> 64 hidden neurons (ReLU) -> 10 outputs (softmax)
// trained by mini-batch stochastic gradient descent (SGD).
//
// The file is organised in the regions students must identify in Part A:
//   [1] Data loading          load_idx(), synthetic()
//   [2] Model initialisation  NN::NN()
//   [3] Forward pass          first half of NN::sample_grad()
//   [4] Backward pass         second half of NN::sample_grad()
//   [5] Mini-batch loop       inner loop of NN::train()   <-- parallel target
//   [6] Weight update         end of the batch in NN::train()
//   [7] Evaluation            NN::accuracy()
//
// Build:  g++ -O2 -std=c++17 nn_sequential.cpp -o nn_sequential
// Run:    ./nn_sequential ../data        (MNIST IDX files)
//         ./nn_sequential --synthetic    (smoke test, no download needed)

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using std::vector;

// ---------------------------------------------------------------------------
// [1] Data loading
// ---------------------------------------------------------------------------

// Read a 32-bit big-endian integer (IDX header format).
static uint32_t be32(std::ifstream& f) {
    unsigned char b[4];
    f.read((char*)b, 4);
    return (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) | (uint32_t(b[2]) << 8) | b[3];
}

struct Data {
    vector<float>   x;        // n * d pixels, row-major, normalised to [0,1]
    vector<uint8_t> y;        // n labels in 0..9
    int n = 0;                // number of samples
    int d = 784;              // pixels per sample
};

// Load at most `limit` samples from an IDX image file and its label file.
Data load_idx(const std::string& ip, const std::string& lp, int limit) {
    std::ifstream fi(ip, std::ios::binary), fl(lp, std::ios::binary);
    if (!fi || !fl) throw std::runtime_error("MNIST files not found");
    if (be32(fi) != 2051 || be32(fl) != 2049) throw std::runtime_error("Bad IDX magic");

    int ni = be32(fi), nl = be32(fl), rows = be32(fi), cols = be32(fi);
    int n = std::min({ni, nl, limit});

    Data d;
    d.n = n;
    d.d = rows * cols;
    d.x.resize((size_t)n * d.d);
    d.y.resize(n);
    vector<unsigned char> buf(d.d);
    for (int i = 0; i < n; i++) {
        fi.read((char*)buf.data(), d.d);
        for (int j = 0; j < d.d; j++) d.x[(size_t)i * d.d + j] = buf[j] / 255.0f;
        fl.read((char*)&d.y[i], 1);
    }
    return d;
}

// Synthetic dataset for smoke tests: class y lights up 20 fixed pixels.
Data synthetic(int n) {
    Data d;
    d.n = n;
    d.x.assign((size_t)n * 784, 0);
    d.y.resize(n);
    std::mt19937 g(1);
    std::normal_distribution<float> noise(0, 0.03f);
    for (int i = 0; i < n; i++) {
        int y = i % 10;
        d.y[i] = y;
        for (int j = 0; j < 784; j++) d.x[(size_t)i * 784 + j] = noise(g);
        for (int k = 0; k < 20; k++) d.x[(size_t)i * 784 + (y * 73 + k) % 784] += 1.0f;
    }
    return d;
}

// ---------------------------------------------------------------------------
// The model
// ---------------------------------------------------------------------------
struct NN {
    int D = 784, H = 64, C = 10;
    // Parameters. Layout: W1[j*H+k] connects input j to hidden k,
    //                     W2[k*C+c] connects hidden k to output c.
    vector<float> W1, b1, W2, b2;

    // [2] Model initialisation: deterministic pseudo-random weights (seed 42).
    NN() {
        std::mt19937 g(42);
        std::normal_distribution<float> nd(0, 0.05f);
        W1.resize(D * H);
        b1.assign(H, 0);
        W2.resize(H * C);
        b2.assign(C, 0);
        for (auto& v : W1) v = nd(g);
        for (auto& v : W2) v = nd(g);
    }

    // Forward + backward pass for ONE sample (x, label).
    // Reads:  W1, b1, W2, b2 (the model)              -> shared, read-only
    // Adds to: g1, gb1, g2, gb2 (gradient accumulators),
    //          loss and correct (statistics)          -> whoever owns them
    // The method is `const`: it never modifies the model.
    void sample_grad(const float* x, int label,
                     vector<float>& g1, vector<float>& gb1,
                     vector<float>& g2, vector<float>& gb2,
                     float& loss, int& correct) const {
        // ---- [3] Forward pass -------------------------------------------
        vector<float> h(H), logit(C), prob(C);

        // Hidden layer: h = ReLU(W1^T x + b1)
        for (int k = 0; k < H; k++) {
            float s = b1[k];
            for (int j = 0; j < D; j++) s += x[j] * W1[j * H + k];
            h[k] = std::max(0.0f, s);
        }

        // Output layer: logits = W2^T h + b2
        float m = -1e30f;
        for (int c = 0; c < C; c++) {
            float s = b2[c];
            for (int k = 0; k < H; k++) s += h[k] * W2[k * C + c];
            logit[c] = s;
            m = std::max(m, s);
        }

        // Softmax (shifted by the max for numerical stability)
        float z = 0;
        for (int c = 0; c < C; c++) {
            prob[c] = std::exp(logit[c] - m);
            z += prob[c];
        }
        for (auto& v : prob) v /= z;

        // Statistics: cross-entropy loss and correct prediction count
        loss -= std::log(std::max(prob[label], 1e-8f));
        correct += int(std::max_element(prob.begin(), prob.end()) - prob.begin()) == label;

        // ---- [4] Backward pass ------------------------------------------
        // dL/dlogit = prob - onehot(label)
        vector<float> dl = prob;
        dl[label] -= 1;

        // Output layer gradients
        for (int k = 0; k < H; k++)
            for (int c = 0; c < C; c++) g2[k * C + c] += h[k] * dl[c];
        for (int c = 0; c < C; c++) gb2[c] += dl[c];

        // Back-propagate to the hidden layer through ReLU
        vector<float> dh(H, 0);
        for (int k = 0; k < H; k++) {
            for (int c = 0; c < C; c++) dh[k] += W2[k * C + c] * dl[c];
            if (h[k] <= 0) dh[k] = 0;
        }

        // Hidden layer gradients (the most expensive loop: 784 x 64)
        for (int j = 0; j < D; j++)
            for (int k = 0; k < H; k++) g1[j * H + k] += x[j] * dh[k];
        for (int k = 0; k < H; k++) gb1[k] += dh[k];
    }

    // Training loop: epochs x mini-batches.
    // P = number of worker threads used to process each mini-batch.
    void train(const Data& d, int epochs, int bs, float lr, int P) {
        for (int e = 0; e < epochs; e++) {
            float L = 0;   // epoch loss
            int ok = 0;    // epoch correct count

            for (int s = 0; s < d.n; s += bs) {
                int e2 = std::min(s + bs, d.n);   // last batch may be shorter
                int n = e2 - s;

                // Batch gradient accumulators, zeroed for every batch.
                vector<float> g1(W1.size()), gb1(H), g2(W2.size()), gb2(C);

                // ---- [5] Mini-batch accumulation, P workers ---------------
                // Part F: split the batch into P contiguous slices (same
                // base/extra scheme as Part C). Each worker computes every
                // sample's gradient into a small LOCAL buffer (unlocked -
                // this is the expensive forward/backward work), then locks
                // mtx only to merge that one sample's contribution into the
                // shared g1/gb1/g2/gb2/L/ok. The lock protects exactly one
                // invariant: a merge is never interrupted midway, so the
                // shared accumulators always equal the sum of a whole
                // number of completed sample contributions.
                int base = n / P, extra = n % P;
                int cursor = s;
                vector<std::thread> workers;
                std::mutex mtx;
                for (int w = 0; w < P; w++) {
                    int count = base + (w < extra ? 1 : 0);
                    int start = cursor, end = cursor + count;
                    cursor = end;
                    workers.emplace_back([&, start, end]() {
                        vector<float> lg1(W1.size()), lgb1(H), lg2(W2.size()), lgb2(C);
                        for (int i = start; i < end; i++) {
                            std::fill(lg1.begin(), lg1.end(), 0.f);
                            std::fill(lgb1.begin(), lgb1.end(), 0.f);
                            std::fill(lg2.begin(), lg2.end(), 0.f);
                            std::fill(lgb2.begin(), lgb2.end(), 0.f);
                            float sLoss = 0; int sOk = 0;
                            sample_grad(&d.x[(size_t)i * d.d], d.y[i], lg1, lgb1, lg2, lgb2, sLoss, sOk);

                            std::lock_guard<std::mutex> lock(mtx);
                            for (size_t k = 0; k < lg1.size(); k++) g1[k] += lg1[k];
                            for (size_t k = 0; k < lgb1.size(); k++) gb1[k] += lgb1[k];
                            for (size_t k = 0; k < lg2.size(); k++) g2[k] += lg2[k];
                            for (size_t k = 0; k < lgb2.size(); k++) gb2[k] += lgb2[k];
                            L += sLoss;
                            ok += sOk;
                        }
                    });
                }
                for (std::thread& t : workers) t.join();

                // ---- [6] Weight update ------------------------------------
                // Performed once per batch, AFTER the full batch gradient
                // is known: W <- W - lr * G / n
                float a = lr / n;
                for (size_t i = 0; i < W1.size(); i++) W1[i] -= a * g1[i];
                for (int i = 0; i < H; i++) b1[i] -= a * gb1[i];
                for (size_t i = 0; i < W2.size(); i++) W2[i] -= a * g2[i];
                for (int i = 0; i < C; i++) b2[i] -= a * gb2[i];
            }

            std::cout << "Epoch " << e + 1 << " loss=" << L / d.n
                      << " train_acc=" << 100.0 * ok / d.n << "%\n";
        }
    }

    // ---- [7] Evaluation ---------------------------------------------------
    // Forward pass only (no softmax needed: argmax of logits == argmax of probs).
    double accuracy(const Data& d) const {
        int ok = 0;
        for (int i = 0; i < d.n; i++) {
            vector<float> h(H), o(C);
            const float* x = &d.x[(size_t)i * d.d];
            for (int k = 0; k < H; k++) {
                float s = b1[k];
                for (int j = 0; j < D; j++) s += x[j] * W1[j * H + k];
                h[k] = std::max(0.0f, s);
            }
            for (int c = 0; c < C; c++) {
                float s = b2[c];
                for (int k = 0; k < H; k++) s += h[k] * W2[k * C + c];
                o[c] = s;
            }
            ok += int(std::max_element(o.begin(), o.end()) - o.begin()) == d.y[i];
        }
        return 100.0 * ok / d.n;
    }
};

// Simple checksum (sum of all weights) to compare runs for Part E/I.
static double checksum(const NN& nn) {
    double s = 0;
    for (float v : nn.W1) s += v;
    for (float v : nn.b1) s += v;
    for (float v : nn.W2) s += v;
    for (float v : nn.b2) s += v;
    return s;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    try {
        bool syn = false;
        std::string root = "../data";
        int threads = 1;

        for (int i = 1; i < argc; i++) {
            std::string a = argv[i];
            if (a == "--synthetic") syn = true;
            else if (a == "--threads" && i + 1 < argc) threads = std::atoi(argv[++i]);
            else root = a;
        }
        if (threads < 1) threads = 1;

        Data tr = syn ? synthetic(2000)
                      : load_idx(root + "/train-images-idx3-ubyte",
                                 root + "/train-labels-idx1-ubyte", 12000);
        Data te = syn ? synthetic(500)
                      : load_idx(root + "/t10k-images-idx3-ubyte",
                                 root + "/t10k-labels-idx1-ubyte", 2000);

        NN nn;
        auto t = std::chrono::steady_clock::now();
        nn.train(tr, /*epochs=*/3, /*batch size=*/64, /*learning rate=*/0.08f, threads);
        double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t).count();

        std::cout << "Threads: " << threads << "\n"
                  << "Test accuracy: " << nn.accuracy(te) << "%\n"
                  << "Weight checksum: " << checksum(nn) << "\n"
                  << "Training time: " << sec << " s\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\nRun data/download_mnist.py or use --synthetic\n";
        return 1;
    }
}
