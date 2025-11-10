#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <limits>
#include <fstream>
#include <string>

using namespace std;
ofstream ofp("output.txt");
int scnt = 1, Mcnt = 1; //몇번째 출력인지 카운트하는 변수
struct Move {
    int r1, c1; // from
    int r2, c2; // to
};

using Board = vector<vector<char>>;

const char HUMAN = 'W';      // 사용자: White
const char COMPUTER = 'B';   // 컴퓨터: Black
const int INF = 1000000000;
const int MAX_DEPTH = 3;

// 보드 출력
void printBoard(const Board& B) {
    cout << "-----------------\n";
    ofp << "Initial Board State #:" << scnt << endl;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            ofp << B[r][c] << " ";
            cout << B[r][c] << " ";
        }
        ofp << "\n\n";
        cout << "\n";
    }
    cout << "-----------------\n";
    scnt++;
}

// 초기 보드 생성
Board initialBoard() {
    Board B(3, vector<char>(3, '.'));  // 3x3 기본 초기화
    ifstream fin("input.txt");

    if (!fin.is_open()) {
        // 파일 열기 실패 시 기본 보드 생성
        B[0] = { 'W', 'W', 'W' };
        B[1] = { '.', '.', '.' };
        B[2] = { 'B', 'B', 'B' };
        return B;
    }

    string line;
    int row = 0;
    bool valid = false;

    while (getline(fin, line) && row < 3) {  // 한 줄씩 읽기
        int col = 0;
        for (char ch : line) {
            if (ch == 'W' || ch == 'B' || ch == '.') {
                B[row][col++] = ch;
                valid = true;
                if (col >= 3) break;
            }
        }
        row++;
    }
    fin.close();

    // 파일은 열렸지만 내용이 없거나 형식이 잘못된 경우 기본 보드로 초기화
    if (!valid) {
        B[0] = { 'W', 'W', 'W' };
        B[1] = { '.', '.', '.' };
        B[2] = { 'B', 'B', 'B' };
    }

    return B;
}

// 현재 말의 이동 가능 여부 판단
vector<Move> generateMoves(const Board& B, char player) {
    vector<Move> moves;
    int dr4[4] = { -1, +1, 0, 0 }; //상하
    int dc4[4] = { 0, 0, -1, +1 }; //좌우
    int front = (player == 'W') ? +1 : -1; // W는 아래(+1), B는 위(-1)

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            if (B[r][c] != player) continue; //빈칸이면 넘어가기

            // 상하좌우 1칸 이동 : 빈칸만 가능
            for (int k = 0; k < 4; ++k) {
                int nr = r + dr4[k];
                int nc = c + dc4[k];
                if (nr >= 0 && nr < 3 && nc >= 0 && nc < 3 && B[nr][nc] == '.') {
                    moves.push_back({ r, c, nr, nc });
                }
            }

            // 대각선 앞 한 칸 kill
            for (int dc = -1; dc <= 1; dc += 2) {
                int nr = r + front;
                int nc = c + dc;
                if (nr >= 0 && nr < 3 && nc >= 0 && nc < 3) {
                    if (B[nr][nc] != '.' && B[nr][nc] != player) {
                        moves.push_back({ r, c, nr, nc });
                    }
                }
            }
        }
    }
    return moves;
}

// 평가함수: White 유리할수록 양수, Black 유리할수록 음수
int evaluate(const Board& B) {
    int Wscore = 0, Bscore = 0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            if (B[r][c] == 'W') {
                int rw = r + 1;
                Wscore += rw * rw;
            }
            else if (B[r][c] == 'B') {
                int rb = r + 1;
                Bscore += (4 - r) * (4 - r);
            }
        }
    }
    return Wscore - Bscore;
}

//종료 조건: 반대편 도착
bool reachedOppositeEnd(const Board& B, char player) {
    if (player == 'W') {
        for (int c = 0; c < 3; ++c) if (B[2][c] == 'W') return true;
    }
    else {
        for (int c = 0; c < 3; ++c) if (B[0][c] == 'B') return true;
    }
    return false;
}

// 말 개수 세기
pair<int, int> countPieces(const Board& B) {
    int w = 0, b = 0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            if (B[r][c] == 'W') ++w;
            else if (B[r][c] == 'B') ++b;
        }
    }
    return { w, b };
}

// 터미널(종료) 여부 판정
bool isTerminal(const Board& B, char player, int& scoreOut) {
    auto [w, b] = countPieces(B);

    // 전멸
    if (w == 0) { scoreOut = -100; return true; } // White 패배
    if (b == 0) { scoreOut = +100; return true; } // Black 패배

    // 반대편 도착
    if (reachedOppositeEnd(B, 'W')) { scoreOut = +100; return true; }
    if (reachedOppositeEnd(B, 'B')) { scoreOut = -100; return true; }

    // 이동 불가 (스테일메이트)
    auto moves = generateMoves(B, player);
    if (moves.empty()) {
        scoreOut = (player == 'W') ? -100 : +100;
        return true;
    }
    return false;
}

// 알파-베타 탐색
int alphabeta(Board B, int depth, int alpha, int beta, bool maximizing) {
    int terminalScore;
    char player = maximizing ? 'W' : 'B';  // 이 턴에 둘 플레이어

    // 3-ply 도달 또는 게임 종료 처리
    if (depth == MAX_DEPTH || isTerminal(B, player, terminalScore)) {
        if (depth == MAX_DEPTH && !isTerminal(B, player, terminalScore))
            return evaluate(B);            // 깊이만 차면 평가함수 사용
        return terminalScore;              // 승/패/스테일메이트면 +-100
    }

    // 현재 턴의 수 생성
    auto moves = generateMoves(B, player);

    if (maximizing) {
        // White 턴: Max
        int value = -INF;
        int subtreeIndex = 1; // 몇번째 서브트리인지
        for (auto m : moves) {
            Board next = B;
            next[m.r2][m.c2] = next[m.r1][m.c1];
            next[m.r1][m.c1] = '.';
            value = max(value, alphabeta(next, depth + 1, alpha, beta, false));
            alpha = max(alpha, value);
            if (alpha >= beta) {
                cout << "[PRUNED] Depth(ply): " << depth
                    << ", Subtree: " << subtreeIndex
                    << ", α=" << alpha << ", β=" << beta << endl;
                ofp << "[PRUNED] Depth(ply): " << depth
                    << ", Subtree: " << subtreeIndex
                    << ", α=" << alpha << ", β=" << beta << endl;
                break;
            }
            subtreeIndex++;
        }
        return value;
    }
    else {
        // Black 턴: Min
        int value = INF;
        int subtreeIndex = 1;
        for (auto m : moves) {
            Board next = B;
            next[m.r2][m.c2] = next[m.r1][m.c1];
            next[m.r1][m.c1] = '.';
            value = min(value, alphabeta(next, depth + 1, alpha, beta, true));
            beta = min(beta, value);
            if (alpha >= beta) {
                cout << "[PRUNED] Depth(ply): " << depth
                    << ", Subtree: " << subtreeIndex
                    << ", α=" << alpha << ", β=" << beta << endl;
                ofp << "[PRUNED] Depth(ply): " << depth
                    << ", Subtree: " << subtreeIndex
                    << ", α=" << alpha << ", β=" << beta << endl;
                break;
            }
            subtreeIndex++;
        }
        return value;
    }
}

/* -------------------- 컴퓨터 수 선택 -------------------- */
bool computerMove(Board& B) {
    auto moves = generateMoves(B, COMPUTER);
    if (moves.empty()) return false;

    int bestVal = INF;
    Move bestMove = moves[0];

    for (auto m : moves) {
        Board next = B;
        next[m.r2][m.c2] = next[m.r1][m.c1];
        next[m.r1][m.c1] = '.';
        int val = alphabeta(next, 1, -INF, INF, true); // White가 다음 턴
        if (val < bestVal) {
            bestVal = val;
            bestMove = m;
        }
    }

    // 실제 이동 적용
    B[bestMove.r2][bestMove.c2] = B[bestMove.r1][bestMove.c1];
    B[bestMove.r1][bestMove.c1] = '.';
    cout << ">> Computer Move: (" << bestMove.r1 + 1 << "," << bestMove.c1 + 1
        << ") -> (" << bestMove.r2 + 1 << "," << bestMove.c2 + 1 << ")\n";
    ofp << ">> Computer Move: (" << bestMove.r1 + 1 << "," << bestMove.c1 + 1
        << ") -> (" << bestMove.r2 + 1 << "," << bestMove.c2 + 1 << ")\n";
    return true;
}

/* -------------------- 사용자 입력 -------------------- */
bool userMove(Board& B) {
    auto moves = generateMoves(B, HUMAN);
    if (moves.empty()) return false;

    while (true) {
        ofp << "Input User's Move #" << Mcnt << endl;
        cout << "From (row col): ";
        int r1, c1;
        cin >> r1 >> c1;
        cout << "To (row col): ";
        int r2, c2;
        cin >> r2 >> c2;
        ofp << "From: " << r1 << " " << c1 << endl << "To: " << r2 << " " << c2 << endl << endl;
        --r1; --c1; --r2; --c2;

        for (auto m : moves) {
            if (m.r1 == r1 && m.c1 == c1 && m.r2 == r2 && m.c2 == c2) {
                B[m.r2][m.c2] = B[m.r1][m.c1];
                B[m.r1][m.c1] = '.';
                return true;
            }
        }
        cout << "Invalid move. Try again.\n";
    }
    Mcnt++;
}

int main() {

    cout << "=====================================\n";
    cout << "        Hexapawn Game (3x3)\n";
    cout << "     User: White  /  Computer: Black\n";
    cout << "=====================================\n\n";

    Board B = initialBoard();
    printBoard(B);

    while (true) {
        int score = 0;

        // 사용자 수
        if (isTerminal(B, 'W', score)) break;
        cout << "\n[User Turn]\n";
        if (!userMove(B)) {
            cout << "User cannot move. Computer wins!\n";
            ofp << "User cannot move. Computer wins!\n";
            break;
        }
        printBoard(B);
        if (isTerminal(B, 'B', score)) break;

        // 컴퓨터 수
        cout << "\n[Computer Turn]\n";
        if (!computerMove(B)) {
            cout << "Computer cannot move. User wins!\n";
            ofp << "Computer cannot move. User wins!\n";
            break;
        }
        ofp << "Next Board State after Computer's Move:" << endl;
        printBoard(B);

        int eval = evaluate(B);
        ofp << "Computer's Evaluation Value: " << eval << "pts\n";
        cout << "Computer Evaluation Value: " << eval << " pts\n";
    }

    cout << "\n=============================\n";
    cout << "         Game Over\n";
    cout << "=============================\n";

    ofp.close();
    return 0;
}
