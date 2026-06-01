// gumcpp - C++ port of gum (shell toolkit)
// Subcommands: choose, confirm, input, filter, format, join, spin, style, write, table, log

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>

static const char* VERSION = "0.1.0";
static const char* R = "\033[0m", *B = "\033[1m", *D = "\033[2m", *C = "\033[36m", *M = "\033[35m", *Y = "\033[33m", *G = "\033[32m", *W = "\033[37m", *RD = "\033[31m";

/* Terminal raw mode */
static struct termios origTerm;
static void enableRaw() { struct termios t; tcgetattr(STDIN_FILENO, &t); origTerm = t; t.c_lflag &= ~(ECHO | ICANON); tcsetattr(STDIN_FILENO, TCSANOW, &t); }
static void disableRaw() { tcsetattr(STDIN_FILENO, TCSANOW, &origTerm); }

/* Terminal size */
static int termW() { struct winsize ws; ioctl(0, TIOCGWINSZ, &ws); return ws.ws_col > 0 ? ws.ws_col : 80; }
static int termH() { struct winsize ws; ioctl(0, TIOCGWINSZ, &ws); return ws.ws_row > 0 ? ws.ws_row : 24; }

/* ---- choose: fuzzy selector ---- */
static void cmdChoose(int argc, char* argv[]) {
    std::vector<std::string> opts;
    int limit = 0, height = 10; std::string header, cursor = "> ";
    bool noLimit = false;

    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--header" && i+1<argc) header = argv[++i];
        else if (a == "--limit" && i+1<argc) limit = atoi(argv[++i]);
        else if (a == "--height" && i+1<argc) height = atoi(argv[++i]);
        else if (a == "--cursor" && i+1<argc) cursor = argv[++i];
        else if (a == "--no-limit") noLimit = true;
        else if (!a.empty() && a[0] != '-') opts.push_back(a);
    }
    /* Read from stdin if no args */
    if (opts.empty()) { std::string line; while (std::getline(std::cin, line)) if (!line.empty()) opts.push_back(line); }
    if (opts.empty()) return;

    int selected = 0;
    std::string filter;
    enableRaw();
    printf("\033[?25l"); /* hide cursor */

    while (true) {
        /* Filter options */
        std::vector<int> matches;
        for (size_t i = 0; i < opts.size(); i++) {
            std::string lo = opts[i]; for (auto& c : lo) c = tolower(c);
            std::string lf = filter; for (auto& c : lf) c = tolower(c);
            if (lf.empty() || lo.find(lf) != std::string::npos) matches.push_back(i);
        }

        /* Render */
        printf("\033[%dA\033[J", height + 2);
        if (!header.empty()) printf("%s\n", header.c_str());
        printf("%sSearch: %s%s\n", cursor.c_str(), filter.c_str(), R);

        int nShow = std::min((int)matches.size(), height);
        for (int j = 0; j < nShow; j++) {
            int idx = matches[j];
            if (idx == selected) printf("%s> %s%s\n", C, opts[idx].c_str(), R);
            else printf("  %s\n", opts[idx].c_str());
        }
        fflush(stdout);

        char c = getchar();
        if (c == 27) { /* ESC */ getchar(); if (getchar() == 'A') { selected--; if (selected < 0) selected = matches.empty() ? 0 : matches.back(); }
            else { selected++; if (selected >= (int)matches.size()) selected = 0; } }
        else if (c == '\n' || c == '\r') {
            if (!matches.empty()) printf("%s\n", opts[matches[selected]].c_str());
            break;
        }
        else if (c == 127 || c == 8) { if (!filter.empty()) filter.pop_back(); }
        else if (c >= 32) filter += c;
    }
    printf("\033[?25h"); /* show cursor */
    disableRaw();
}

/* ---- confirm: yes/no prompt ---- */
static void cmdConfirm(int argc, char* argv[]) {
    std::string prompt = "Are you sure?";
    bool affirmative = false, defaultYes = true;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--affirmative") affirmative = true;
        else if (a == "--default" && i+1<argc) defaultYes = (strcmp(argv[++i], "Yes") == 0);
        else if (!a.empty() && a[0] != '-') prompt = a;
    }
    printf("%s%s (y/N) %s", prompt.c_str(), defaultYes ? " [Y/n]" : " [y/N]", R); fflush(stdout);
    char c = getchar();
    bool yes = (c == 'y' || c == 'Y' || (c == '\n' && defaultYes));
    if (affirmative && yes) printf("Yes\n");
    exit(yes ? 0 : 1);
}

/* ---- input: text input ---- */
static void cmdInput(int argc, char* argv[]) {
    std::string placeholder = "", prompt = "> ", value;
    bool password = false; int width = 0, charLimit = 0;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--placeholder" && i+1<argc) placeholder = argv[++i];
        else if (a == "--prompt" && i+1<argc) prompt = argv[++i];
        else if (a == "--value" && i+1<argc) value = argv[++i];
        else if (a == "--password") password = true;
        else if (a == "--width" && i+1<argc) width = atoi(argv[++i]);
        else if (a == "--char-limit" && i+1<argc) charLimit = atoi(argv[++i]);
    }
    printf("%s", prompt.c_str()); fflush(stdout);
    if (!value.empty()) printf("%s", value.c_str());
    enableRaw(); std::string input = value;
    while (true) {
        printf("\r\033[K%s%s", prompt.c_str(), password ? std::string(input.size(), '*').c_str() : input.c_str());
        if (!input.empty() || placeholder.empty()) {}
        else printf("%s%s%s", D, placeholder.c_str(), R);
        fflush(stdout);
        char c = getchar();
        if (c == '\n' || c == '\r') break;
        if (c == 127 || c == 8) { if (!input.empty()) input.pop_back(); }
        else if (c >= 32 && (charLimit == 0 || (int)input.size() < charLimit)) input += c;
    }
    printf("\n");
    disableRaw();
    printf("%s\n", input.c_str());
}

/* ---- filter: interactive text filter ---- */
static void cmdFilter(int argc, char* argv[]) {
    std::string placeholder = "Filter...", initial = ""; bool reverse = false;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--placeholder" && i+1<argc) placeholder = argv[++i];
        else if (a == "--value" && i+1<argc) initial = argv[++i];
        else if (a == "--reverse") reverse = true;
    }
    std::vector<std::string> lines; std::string line;
    while (std::getline(std::cin, line)) if (!line.empty()) lines.push_back(line);
    if (lines.empty()) return;

    std::string filter = initial;
    enableRaw();
    while (true) {
        printf("\033[2J\033[H");
        printf("%s%s: %s%s\n", C, placeholder.c_str(), filter.c_str(), R);
        int shown = 0;
        for (auto& l : lines) {
            std::string lo = l; for (auto& c : lo) c = tolower(c);
            std::string lf = filter; for (auto& c : lf) c = tolower(c);
            if (lf.empty() || lo.find(lf) != std::string::npos) {
                printf("%s\n", l.c_str());
                if (++shown >= termH() - 2) break;
            }
        }
        fflush(stdout);
        char c = getchar();
        if (c == 27) break; /* ESC */
        if (c == 127 || c == 8) { if (!filter.empty()) filter.pop_back(); }
        else if (c >= 32) filter += c;
    }
    disableRaw();
}

/* ---- format: ANSI text formatting ---- */
static void cmdFormat(int argc, char* argv[]) {
    std::string type = "markdown";
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-t" || a == "--type" && i+1<argc) type = argv[++i];
    }
    std::string input; char buf[4096];
    while (fgets(buf, sizeof(buf), stdin)) input += buf;
    /* Simple formatting */
    if (type == "code") printf("%s%s%s\n", D, input.c_str(), R);
    else if (type == "emoji") printf("%s\n", input.c_str());
    else printf("%s\n", input.c_str());
}

/* ---- join: join lines ---- */
static void cmdJoin(int argc, char* argv[]) {
    std::string sep = " ", prefix, suffix;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--separator" && i+1<argc) sep = argv[++i];
        else if (a == "--prefix" && i+1<argc) prefix = argv[++i];
        else if (a == "--suffix" && i+1<argc) suffix = argv[++i];
    }
    std::vector<std::string> lines; std::string line;
    while (std::getline(std::cin, line)) if (!line.empty()) lines.push_back(line);
    for (size_t i = 0; i < lines.size(); i++) {
        if (i > 0) printf("%s", sep.c_str());
        printf("%s%s%s", prefix.c_str(), lines[i].c_str(), suffix.c_str());
    }
    printf("\n");
}

/* ---- spin: spinner ---- */
static void cmdSpin(int argc, char* argv[]) {
    std::string title = "Loading...", cmd;
    std::string spinner = "⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏";
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--title" && i+1<argc) title = argv[++i];
        else if (a == "--spinner" && i+1<argc) spinner = argv[++i];
        else if (a == "--" && i+1<argc) { cmd = argv[++i]; break; }
        else if (a[0] != '-') { cmd = a; for (int j = i+1; j < argc; j++) cmd += " " + std::string(argv[j]); break; }
    }
    if (cmd.empty()) return;

    int si = 0;
    printf("\033[?25l");
    bool done = false;
    std::thread t([&]() {
        while (!done) {
            printf("\r%s%c%s %s%s", C, spinner[si % spinner.size()], R, title.c_str(), D);
            fflush(stdout); si++;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    int ret = system(cmd.c_str());
    done = true; t.join();
    printf("\r\033[K%s%s%s\n", ret == 0 ? G : RD, ret == 0 ? "✓" : "✗", R);
    printf("\033[?25h");
    exit(ret);
}

/* ---- style: apply borders/colors ---- */
static void cmdStyle(int argc, char* argv[]) {
    std::string fg, bg, border = "none", padding, margin, align = "left";
    int width = 0, height = 0;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--foreground" && i+1<argc) fg = argv[++i];
        else if (a == "--background" && i+1<argc) bg = argv[++i];
        else if (a == "--border" && i+1<argc) border = argv[++i];
        else if (a == "--width" && i+1<argc) width = atoi(argv[++i]);
        else if (a == "--height" && i+1<argc) height = atoi(argv[++i]);
        else if (a == "--align" && i+1<argc) align = argv[++i];
    }
    std::string input; char buf[4096];
    while (fgets(buf, sizeof(buf), stdin)) input += buf;

    /* Apply foreground color */
    std::string pre;
    if (fg == "red" || fg == "1") pre += "\033[31m";
    else if (fg == "green" || fg == "2") pre += "\033[32m";
    else if (fg == "yellow" || fg == "3") pre += "\033[33m";
    else if (fg == "blue" || fg == "4") pre += "\033[34m";
    else if (fg == "magenta" || fg == "5") pre += "\033[35m";
    else if (fg == "cyan" || fg == "6") pre += "\033[36m";

    if (border == "rounded") {
        printf("╭──\n│ %s%s%s\n╰──\n", pre.c_str(), input.c_str(), R);
    } else {
        printf("%s%s%s", pre.c_str(), input.c_str(), R);
    }
}

/* ---- write: multi-line text editor ---- */
static void cmdWrite(int argc, char* argv[]) {
    std::string showCursor = "", placeholder = "";
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--show-cursor-line") showCursor = "true";
        else if (a == "--placeholder" && i+1<argc) placeholder = argv[++i];
    }
    printf("%s%s\n(Enter text, press Ctrl+D when done)\n%s", D, placeholder.c_str(), R);
    std::string content; char buf[4096];
    while (fgets(buf, sizeof(buf), stdin)) content += buf;
    printf("\n%s", content.c_str());
}

/* ---- table: render CSV/TSV as table ---- */
static void cmdTable(int argc, char* argv[]) {
    std::string sep = ",", file;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-s" || a == "--separator" && i+1<argc) sep = argv[++i];
        else if (a == "-f" || a == "--file" && i+1<argc) file = argv[++i];
    }
    /* Read input */
    std::vector<std::vector<std::string>> rows;
    auto& input = std::cin;
    /* If file specified, open it */
    std::string line;
    if (!file.empty()) {
        FILE* f = fopen(file.c_str(), "r");
        if (!f) return;
        char lbuf[4096];
        while (fgets(lbuf, sizeof(lbuf), f)) {
            line = lbuf; while (!line.empty() && line.back()=='\n') line.pop_back();
            std::vector<std::string> row;
            size_t pos = 0;
            while (pos <= line.size()) {
                size_t c = line.find(sep, pos);
                row.push_back((c==std::string::npos)?line.substr(pos):line.substr(pos,c-pos));
                if (c==std::string::npos) break; pos = c+1;
            }
            rows.push_back(row);
        }
        fclose(f);
    } else {
        while (std::getline(std::cin, line)) {
            while (!line.empty() && line.back()=='\n') line.pop_back();
            std::vector<std::string> row;
            size_t pos = 0;
            while (pos <= line.size()) {
                size_t c = line.find(sep, pos);
                row.push_back((c==std::string::npos)?line.substr(pos):line.substr(pos,c-pos));
                if (c==std::string::npos) break; pos = c+1;
            }
            rows.push_back(row);
        }
    }
    if (rows.empty()) return;

    /* Compute column widths */
    size_t ncols = 0; for (auto& r : rows) if (r.size() > ncols) ncols = r.size();
    std::vector<int> cw(ncols);
    for (auto& r : rows) for (size_t i = 0; i < r.size(); i++) if ((int)r[i].size() > cw[i]) cw[i] = r[i].size();

    /* Print */
    for (auto& r : rows) {
        for (size_t i = 0; i < ncols; i++) {
            if (i > 0) printf(" │ ");
            printf("%-*s", cw[i], i < r.size() ? r[i].c_str() : "");
        }
        printf("\n");
    }
}

/* ---- log: pretty logging ---- */
static void cmdLog(int argc, char* argv[]) {
    std::string level = "info", prefix, msg;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-l" || a == "--level" && i+1<argc) level = argv[++i];
        else if (!a.empty() && a[0] != '-') msg = a;
    }
    if (msg.empty()) { char buf[4096]; while (fgets(buf, sizeof(buf), stdin)) msg += buf; }
    const char* color = G, *label = "INFO";
    if (level == "error") { color = RD; label = "ERROR"; }
    else if (level == "warn") { color = Y; label = "WARN"; }
    else if (level == "debug") { color = D; label = "DEBUG"; }
    else if (level == "fatal") { color = RD; label = "FATAL"; }

    time_t t = time(nullptr); struct tm* tm = localtime(&t);
    char ts[20]; strftime(ts, sizeof(ts), "%H:%M:%S", tm);
    printf("%s%s %s%s %s%s\n", D, ts, color, label, R, msg.c_str());
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "gumcpp %s - shell toolkit\n", VERSION);
        fprintf(stderr, "Usage: gumcpp <command> [flags]\n");
        fprintf(stderr, "Commands: choose confirm input filter format join spin style write table log\n");
        return 1;
    }
    std::string cmd = argv[1];
    if (cmd == "choose") cmdChoose(argc, argv);
    else if (cmd == "confirm") cmdConfirm(argc, argv);
    else if (cmd == "input") cmdInput(argc, argv);
    else if (cmd == "filter") cmdFilter(argc, argv);
    else if (cmd == "format") cmdFormat(argc, argv);
    else if (cmd == "join") cmdJoin(argc, argv);
    else if (cmd == "spin") cmdSpin(argc, argv);
    else if (cmd == "style") cmdStyle(argc, argv);
    else if (cmd == "write") cmdWrite(argc, argv);
    else if (cmd == "table") cmdTable(argc, argv);
    else if (cmd == "log") cmdLog(argc, argv);
    else if (cmd == "version") printf("gumcpp %s\n", VERSION);
    else { fprintf(stderr, "Unknown command: %s\n", cmd.c_str()); return 1; }
    return 0;
}
