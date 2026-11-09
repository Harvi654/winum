/* Original, AI-assisted (Codex), LGPL-2.1-or-later. */
#define THREADS 4
#define COMMAND (WM_APP + 31)
enum operation { FOCUS, ACTIVATE, CAPTURE, RELEASE, CHILD_CREATE, CHILD_FOCUS,
                 CHILD_UNPARENT, CHILD_DESTROY, STOP };
struct worker
{
    HANDLE thread, ready;
    DWORD tid;
    HWND window, children[2];
    unsigned int index;
};
struct edge { unsigned int from, to; };
struct graph
{
    const char *name;
    unsigned int count, cuts;
    struct edge edges[5], remove[3];
    int exit_worker;
    unsigned int other_anchor;
};
static struct worker workers[THREADS];
static const char *scenario;
static unsigned int completed_scenarios, detach_call;
static LONG attachment_errors;

static const struct graph graphs[] =
{
    { "isolated-nonedge", 0, 1, {{0,0}}, {{0,1}}, -1, 1 },
    { "pair", 1, 1, {{0,1}}, {{0,1}}, -1, 1 },
    { "chain", 2, 1, {{0,1},{1,2}}, {{1,2}}, -1, 2 },
    { "bridge", 3, 1, {{0,1},{1,2},{2,3}}, {{1,2}}, -1, 3 },
    { "cycle", 3, 2, {{0,1},{1,2},{2,0}}, {{0,1},{1,2}}, -1, 2 },
    { "duplicate", 2, 3, {{0,1},{0,1}}, {{0,1},{0,1},{0,1}}, -1, 1 },
    { "opposite-edges", 2, 2, {{0,1},{1,0}}, {{0,1},{1,0}}, -1, 1 },
    { "connected-nonedge", 2, 1, {{0,1},{1,2}}, {{0,2}}, -1, 2 },
    { "self-detach", 1, 1, {{0,1}}, {{0,0}}, -1, 1 },
    { "exit-neighbor", 2, 0, {{0,1},{1,2}}, {{0,0}}, 2, 2 },
};

struct expected_state
{
    const char *scenario, *stage, *active, *focus, *capture;
};

/* Literal Windows observations; . is NULL, a/b are C's child windows,
 * and - is an exited thread. No graph algorithm generates the oracle. */
static const struct expected_state expected_states[] =
{
    { "isolated-nonedge-reverse0-anchorA", "isolated", "A...", "A...", "...." },
    { "isolated-nonedge-reverse0-anchorA", "joined", "A...", "A...", "...." },
    { "isolated-nonedge-reverse0-anchorA", "joined-probe-A", "A...", "A...", "...." },
    { "isolated-nonedge-reverse0-anchorA", "joined-probe-B", "AB..", "AB..", "...." },
    { "isolated-nonedge-reverse0-anchorA", "joined-probe-C", "ABC.", "ABC.", "...." },
    { "isolated-nonedge-reverse0-anchorA", "joined-probe-D", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorA", "cut0-before", "ABCD", "ABCD", "A..." },
    { "isolated-nonedge-reverse0-anchorA", "cut0-passive", "ABCD", "ABCD", "A..." },
    { "isolated-nonedge-reverse0-anchorA", "cut0-probe-A", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorA", "cut0-probe-B", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorA", "cut0-probe-C", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorA", "cut0-probe-D", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorB", "isolated", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorB", "joined", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorB", "joined-probe-A", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorB", "joined-probe-B", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorB", "joined-probe-C", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorB", "joined-probe-D", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorB", "cut0-before", "ABCD", "ABCD", ".B.." },
    { "isolated-nonedge-reverse0-anchorB", "cut0-passive", "ABCD", "ABCD", ".B.." },
    { "isolated-nonedge-reverse0-anchorB", "cut0-probe-A", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorB", "cut0-probe-B", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorB", "cut0-probe-C", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse0-anchorB", "cut0-probe-D", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorA", "joined", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorA", "joined-probe-A", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorA", "joined-probe-B", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorA", "joined-probe-C", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorA", "joined-probe-D", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorA", "cut0-before", "ABCD", "ABCD", "A..." },
    { "isolated-nonedge-reverse1-anchorA", "cut0-passive", "ABCD", "ABCD", "A..." },
    { "isolated-nonedge-reverse1-anchorA", "cut0-probe-A", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorA", "cut0-probe-B", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorA", "cut0-probe-C", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorA", "cut0-probe-D", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorB", "isolated", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorB", "joined", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorB", "joined-probe-A", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorB", "joined-probe-B", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorB", "joined-probe-C", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorB", "joined-probe-D", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorB", "cut0-before", "ABCD", "ABCD", ".B.." },
    { "isolated-nonedge-reverse1-anchorB", "cut0-passive", "ABCD", "ABCD", ".B.." },
    { "isolated-nonedge-reverse1-anchorB", "cut0-probe-A", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorB", "cut0-probe-B", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorB", "cut0-probe-C", "ABCD", "ABCD", "...." },
    { "isolated-nonedge-reverse1-anchorB", "cut0-probe-D", "ABCD", "ABCD", "...." },
    { "pair-reverse0-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "pair-reverse0-anchorA", "joined", "BBCD", "BBCD", "...." },
    { "pair-reverse0-anchorA", "joined-probe-A", "AACD", "AACD", "...." },
    { "pair-reverse0-anchorA", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "pair-reverse0-anchorA", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "pair-reverse0-anchorA", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "pair-reverse0-anchorA", "cut0-before", "AACD", "AACD", "AA.." },
    { "pair-reverse0-anchorA", "cut0-passive", "A.CD", "A.CD", "A..." },
    { "pair-reverse0-anchorA", "cut0-probe-A", "A.CD", "A.CD", "...." },
    { "pair-reverse0-anchorA", "cut0-probe-B", "ABCD", "ABCD", "...." },
    { "pair-reverse0-anchorA", "cut0-probe-C", "ABCD", "ABCD", "...." },
    { "pair-reverse0-anchorA", "cut0-probe-D", "ABCD", "ABCD", "...." },
    { "pair-reverse0-anchorB", "isolated", "ABCD", "ABCD", "...." },
    { "pair-reverse0-anchorB", "joined", "BBCD", "BBCD", "...." },
    { "pair-reverse0-anchorB", "joined-probe-A", "AACD", "AACD", "...." },
    { "pair-reverse0-anchorB", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "pair-reverse0-anchorB", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "pair-reverse0-anchorB", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "pair-reverse0-anchorB", "cut0-before", "BBCD", "BBCD", "BB.." },
    { "pair-reverse0-anchorB", "cut0-passive", ".BCD", ".BCD", ".B.." },
    { "pair-reverse0-anchorB", "cut0-probe-A", "ABCD", "ABCD", "...." },
    { "pair-reverse0-anchorB", "cut0-probe-B", "ABCD", "ABCD", "...." },
    { "pair-reverse0-anchorB", "cut0-probe-C", "ABCD", "ABCD", "...." },
    { "pair-reverse0-anchorB", "cut0-probe-D", "ABCD", "ABCD", "...." },
    { "pair-reverse1-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "pair-reverse1-anchorA", "joined", "BBCD", "BBCD", "...." },
    { "pair-reverse1-anchorA", "joined-probe-A", "AACD", "AACD", "...." },
    { "pair-reverse1-anchorA", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "pair-reverse1-anchorA", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "pair-reverse1-anchorA", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "pair-reverse1-anchorA", "cut0-before", "AACD", "AACD", "AA.." },
    { "pair-reverse1-anchorA", "cut0-passive", "A.CD", "A.CD", "A..." },
    { "pair-reverse1-anchorA", "cut0-probe-A", "A.CD", "A.CD", "...." },
    { "pair-reverse1-anchorA", "cut0-probe-B", "ABCD", "ABCD", "...." },
    { "pair-reverse1-anchorA", "cut0-probe-C", "ABCD", "ABCD", "...." },
    { "pair-reverse1-anchorA", "cut0-probe-D", "ABCD", "ABCD", "...." },
    { "pair-reverse1-anchorB", "isolated", "ABCD", "ABCD", "...." },
    { "pair-reverse1-anchorB", "joined", "BBCD", "BBCD", "...." },
    { "pair-reverse1-anchorB", "joined-probe-A", "AACD", "AACD", "...." },
    { "pair-reverse1-anchorB", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "pair-reverse1-anchorB", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "pair-reverse1-anchorB", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "pair-reverse1-anchorB", "cut0-before", "BBCD", "BBCD", "BB.." },
    { "pair-reverse1-anchorB", "cut0-passive", ".BCD", ".BCD", ".B.." },
    { "pair-reverse1-anchorB", "cut0-probe-A", "ABCD", "ABCD", "...." },
    { "pair-reverse1-anchorB", "cut0-probe-B", "ABCD", "ABCD", "...." },
    { "pair-reverse1-anchorB", "cut0-probe-C", "ABCD", "ABCD", "...." },
    { "pair-reverse1-anchorB", "cut0-probe-D", "ABCD", "ABCD", "...." },
    { "chain-reverse0-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "chain-reverse0-anchorA", "joined", "CCCD", "CCCD", "...." },
    { "chain-reverse0-anchorA", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "chain-reverse0-anchorA", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "chain-reverse0-anchorA", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "chain-reverse0-anchorA", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "chain-reverse0-anchorA", "cut0-before", "AAAD", "AAAD", "AAA." },
    { "chain-reverse0-anchorA", "cut0-passive", "AA.D", "AA.D", "AA.." },
    { "chain-reverse0-anchorA", "cut0-probe-A", "AA.D", "AA.D", "...." },
    { "chain-reverse0-anchorA", "cut0-probe-B", "BB.D", "BB.D", "...." },
    { "chain-reverse0-anchorA", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "chain-reverse0-anchorA", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "chain-reverse0-anchorC", "isolated", "ABCD", "ABCD", "...." },
    { "chain-reverse0-anchorC", "joined", "CCCD", "CCCD", "...." },
    { "chain-reverse0-anchorC", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "chain-reverse0-anchorC", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "chain-reverse0-anchorC", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "chain-reverse0-anchorC", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "chain-reverse0-anchorC", "cut0-before", "CCCD", "CCCD", "CCC." },
    { "chain-reverse0-anchorC", "cut0-passive", "..CD", "..CD", "..C." },
    { "chain-reverse0-anchorC", "cut0-probe-A", "AACD", "AACD", "...." },
    { "chain-reverse0-anchorC", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "chain-reverse0-anchorC", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "chain-reverse0-anchorC", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "chain-reverse1-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "chain-reverse1-anchorA", "joined", "CCCD", "CCCD", "...." },
    { "chain-reverse1-anchorA", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "chain-reverse1-anchorA", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "chain-reverse1-anchorA", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "chain-reverse1-anchorA", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "chain-reverse1-anchorA", "cut0-before", "AAAD", "AAAD", "AAA." },
    { "chain-reverse1-anchorA", "cut0-passive", "AA.D", "AA.D", "AA.." },
    { "chain-reverse1-anchorA", "cut0-probe-A", "AA.D", "AA.D", "...." },
    { "chain-reverse1-anchorA", "cut0-probe-B", "BB.D", "BB.D", "...." },
    { "chain-reverse1-anchorA", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "chain-reverse1-anchorA", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "chain-reverse1-anchorC", "isolated", "ABCD", "ABCD", "...." },
    { "chain-reverse1-anchorC", "joined", "CCCD", "CCCD", "...." },
    { "chain-reverse1-anchorC", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "chain-reverse1-anchorC", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "chain-reverse1-anchorC", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "chain-reverse1-anchorC", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "chain-reverse1-anchorC", "cut0-before", "CCCD", "CCCD", "CCC." },
    { "chain-reverse1-anchorC", "cut0-passive", "..CD", "..CD", "..C." },
    { "chain-reverse1-anchorC", "cut0-probe-A", "AACD", "AACD", "...." },
    { "chain-reverse1-anchorC", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "chain-reverse1-anchorC", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "chain-reverse1-anchorC", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "bridge-reverse0-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "bridge-reverse0-anchorA", "joined", "DDDD", "DDDD", "...." },
    { "bridge-reverse0-anchorA", "joined-probe-A", "AAAA", "AAAA", "...." },
    { "bridge-reverse0-anchorA", "joined-probe-B", "BBBB", "BBBB", "...." },
    { "bridge-reverse0-anchorA", "joined-probe-C", "CCCC", "CCCC", "...." },
    { "bridge-reverse0-anchorA", "joined-probe-D", "DDDD", "DDDD", "...." },
    { "bridge-reverse0-anchorA", "cut0-before", "AAAA", "AAAA", "AAAA" },
    { "bridge-reverse0-anchorA", "cut0-passive", "AA..", "AA..", "AA.." },
    { "bridge-reverse0-anchorA", "cut0-probe-A", "AA..", "AA..", "...." },
    { "bridge-reverse0-anchorA", "cut0-probe-B", "BB..", "BB..", "...." },
    { "bridge-reverse0-anchorA", "cut0-probe-C", "BBCC", "BBCC", "...." },
    { "bridge-reverse0-anchorA", "cut0-probe-D", "BBDD", "BBDD", "...." },
    { "bridge-reverse0-anchorD", "isolated", "ABCD", "ABCD", "...." },
    { "bridge-reverse0-anchorD", "joined", "DDDD", "DDDD", "...." },
    { "bridge-reverse0-anchorD", "joined-probe-A", "AAAA", "AAAA", "...." },
    { "bridge-reverse0-anchorD", "joined-probe-B", "BBBB", "BBBB", "...." },
    { "bridge-reverse0-anchorD", "joined-probe-C", "CCCC", "CCCC", "...." },
    { "bridge-reverse0-anchorD", "joined-probe-D", "DDDD", "DDDD", "...." },
    { "bridge-reverse0-anchorD", "cut0-before", "DDDD", "DDDD", "DDDD" },
    { "bridge-reverse0-anchorD", "cut0-passive", "..DD", "..DD", "..DD" },
    { "bridge-reverse0-anchorD", "cut0-probe-A", "AADD", "AADD", "...." },
    { "bridge-reverse0-anchorD", "cut0-probe-B", "BBDD", "BBDD", "...." },
    { "bridge-reverse0-anchorD", "cut0-probe-C", "BBCC", "BBCC", "...." },
    { "bridge-reverse0-anchorD", "cut0-probe-D", "BBDD", "BBDD", "...." },
    { "bridge-reverse1-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "bridge-reverse1-anchorA", "joined", "DDDD", "DDDD", "...." },
    { "bridge-reverse1-anchorA", "joined-probe-A", "AAAA", "AAAA", "...." },
    { "bridge-reverse1-anchorA", "joined-probe-B", "BBBB", "BBBB", "...." },
    { "bridge-reverse1-anchorA", "joined-probe-C", "CCCC", "CCCC", "...." },
    { "bridge-reverse1-anchorA", "joined-probe-D", "DDDD", "DDDD", "...." },
    { "bridge-reverse1-anchorA", "cut0-before", "AAAA", "AAAA", "AAAA" },
    { "bridge-reverse1-anchorA", "cut0-passive", "AA..", "AA..", "AA.." },
    { "bridge-reverse1-anchorA", "cut0-probe-A", "AA..", "AA..", "...." },
    { "bridge-reverse1-anchorA", "cut0-probe-B", "BB..", "BB..", "...." },
    { "bridge-reverse1-anchorA", "cut0-probe-C", "BBCC", "BBCC", "...." },
    { "bridge-reverse1-anchorA", "cut0-probe-D", "BBDD", "BBDD", "...." },
    { "bridge-reverse1-anchorD", "isolated", "ABCD", "ABCD", "...." },
    { "bridge-reverse1-anchorD", "joined", "DDDD", "DDDD", "...." },
    { "bridge-reverse1-anchorD", "joined-probe-A", "AAAA", "AAAA", "...." },
    { "bridge-reverse1-anchorD", "joined-probe-B", "BBBB", "BBBB", "...." },
    { "bridge-reverse1-anchorD", "joined-probe-C", "CCCC", "CCCC", "...." },
    { "bridge-reverse1-anchorD", "joined-probe-D", "DDDD", "DDDD", "...." },
    { "bridge-reverse1-anchorD", "cut0-before", "DDDD", "DDDD", "DDDD" },
    { "bridge-reverse1-anchorD", "cut0-passive", "..DD", "..DD", "..DD" },
    { "bridge-reverse1-anchorD", "cut0-probe-A", "AADD", "AADD", "...." },
    { "bridge-reverse1-anchorD", "cut0-probe-B", "BBDD", "BBDD", "...." },
    { "bridge-reverse1-anchorD", "cut0-probe-C", "BBCC", "BBCC", "...." },
    { "bridge-reverse1-anchorD", "cut0-probe-D", "BBDD", "BBDD", "...." },
    { "cycle-reverse0-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "cycle-reverse0-anchorA", "joined", "CCCD", "CCCD", "...." },
    { "cycle-reverse0-anchorA", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "cycle-reverse0-anchorA", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "cycle-reverse0-anchorA", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "cycle-reverse0-anchorA", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "cycle-reverse0-anchorA", "cut0-before", "AAAD", "AAAD", "AAA." },
    { "cycle-reverse0-anchorA", "cut0-passive", "AAAD", "AAAD", "AAA." },
    { "cycle-reverse0-anchorA", "cut0-probe-A", "AAAD", "AAAD", "...." },
    { "cycle-reverse0-anchorA", "cut0-probe-B", "BBBD", "BBBD", "...." },
    { "cycle-reverse0-anchorA", "cut0-probe-C", "CCCD", "CCCD", "...." },
    { "cycle-reverse0-anchorA", "cut0-probe-D", "CCCD", "CCCD", "...." },
    { "cycle-reverse0-anchorA", "cut1-before", "AAAD", "AAAD", "AAA." },
    { "cycle-reverse0-anchorA", "cut1-passive", "A.AD", "A.AD", "A.A." },
    { "cycle-reverse0-anchorA", "cut1-probe-A", "A.AD", "A.AD", "...." },
    { "cycle-reverse0-anchorA", "cut1-probe-B", "ABAD", "ABAD", "...." },
    { "cycle-reverse0-anchorA", "cut1-probe-C", "CBCD", "CBCD", "...." },
    { "cycle-reverse0-anchorA", "cut1-probe-D", "CBCD", "CBCD", "...." },
    { "cycle-reverse0-anchorC", "isolated", "ABCD", "ABCD", "...." },
    { "cycle-reverse0-anchorC", "joined", "CCCD", "CCCD", "...." },
    { "cycle-reverse0-anchorC", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "cycle-reverse0-anchorC", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "cycle-reverse0-anchorC", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "cycle-reverse0-anchorC", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "cycle-reverse0-anchorC", "cut0-before", "CCCD", "CCCD", "CCC." },
    { "cycle-reverse0-anchorC", "cut0-passive", "CCCD", "CCCD", "CCC." },
    { "cycle-reverse0-anchorC", "cut0-probe-A", "AAAD", "AAAD", "...." },
    { "cycle-reverse0-anchorC", "cut0-probe-B", "BBBD", "BBBD", "...." },
    { "cycle-reverse0-anchorC", "cut0-probe-C", "CCCD", "CCCD", "...." },
    { "cycle-reverse0-anchorC", "cut0-probe-D", "CCCD", "CCCD", "...." },
    { "cycle-reverse0-anchorC", "cut1-before", "CCCD", "CCCD", "CCC." },
    { "cycle-reverse0-anchorC", "cut1-passive", "C.CD", "C.CD", "C.C." },
    { "cycle-reverse0-anchorC", "cut1-probe-A", "A.AD", "A.AD", "...." },
    { "cycle-reverse0-anchorC", "cut1-probe-B", "ABAD", "ABAD", "...." },
    { "cycle-reverse0-anchorC", "cut1-probe-C", "CBCD", "CBCD", "...." },
    { "cycle-reverse0-anchorC", "cut1-probe-D", "CBCD", "CBCD", "...." },
    { "cycle-reverse1-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "cycle-reverse1-anchorA", "joined", "CCCD", "CCCD", "...." },
    { "cycle-reverse1-anchorA", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "cycle-reverse1-anchorA", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "cycle-reverse1-anchorA", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "cycle-reverse1-anchorA", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "cycle-reverse1-anchorA", "cut0-before", "AAAD", "AAAD", "AAA." },
    { "cycle-reverse1-anchorA", "cut0-passive", "AAAD", "AAAD", "AAA." },
    { "cycle-reverse1-anchorA", "cut0-probe-A", "AAAD", "AAAD", "...." },
    { "cycle-reverse1-anchorA", "cut0-probe-B", "BBBD", "BBBD", "...." },
    { "cycle-reverse1-anchorA", "cut0-probe-C", "CCCD", "CCCD", "...." },
    { "cycle-reverse1-anchorA", "cut0-probe-D", "CCCD", "CCCD", "...." },
    { "cycle-reverse1-anchorA", "cut1-before", "AAAD", "AAAD", "AAA." },
    { "cycle-reverse1-anchorA", "cut1-passive", "A.AD", "A.AD", "A.A." },
    { "cycle-reverse1-anchorA", "cut1-probe-A", "A.AD", "A.AD", "...." },
    { "cycle-reverse1-anchorA", "cut1-probe-B", "ABAD", "ABAD", "...." },
    { "cycle-reverse1-anchorA", "cut1-probe-C", "CBCD", "CBCD", "...." },
    { "cycle-reverse1-anchorA", "cut1-probe-D", "CBCD", "CBCD", "...." },
    { "cycle-reverse1-anchorC", "isolated", "ABCD", "ABCD", "...." },
    { "cycle-reverse1-anchorC", "joined", "CCCD", "CCCD", "...." },
    { "cycle-reverse1-anchorC", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "cycle-reverse1-anchorC", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "cycle-reverse1-anchorC", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "cycle-reverse1-anchorC", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "cycle-reverse1-anchorC", "cut0-before", "CCCD", "CCCD", "CCC." },
    { "cycle-reverse1-anchorC", "cut0-passive", "CCCD", "CCCD", "CCC." },
    { "cycle-reverse1-anchorC", "cut0-probe-A", "AAAD", "AAAD", "...." },
    { "cycle-reverse1-anchorC", "cut0-probe-B", "BBBD", "BBBD", "...." },
    { "cycle-reverse1-anchorC", "cut0-probe-C", "CCCD", "CCCD", "...." },
    { "cycle-reverse1-anchorC", "cut0-probe-D", "CCCD", "CCCD", "...." },
    { "cycle-reverse1-anchorC", "cut1-before", "CCCD", "CCCD", "CCC." },
    { "cycle-reverse1-anchorC", "cut1-passive", "C.CD", "C.CD", "C.C." },
    { "cycle-reverse1-anchorC", "cut1-probe-A", "A.AD", "A.AD", "...." },
    { "cycle-reverse1-anchorC", "cut1-probe-B", "ABAD", "ABAD", "...." },
    { "cycle-reverse1-anchorC", "cut1-probe-C", "CBCD", "CBCD", "...." },
    { "cycle-reverse1-anchorC", "cut1-probe-D", "CBCD", "CBCD", "...." },
    { "duplicate-reverse0-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorA", "joined", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorA", "joined-probe-A", "AACD", "AACD", "...." },
    { "duplicate-reverse0-anchorA", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorA", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorA", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorA", "cut0-before", "AACD", "AACD", "AA.." },
    { "duplicate-reverse0-anchorA", "cut0-passive", "AACD", "AACD", "AA.." },
    { "duplicate-reverse0-anchorA", "cut0-probe-A", "AACD", "AACD", "...." },
    { "duplicate-reverse0-anchorA", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorA", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorA", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorA", "cut1-before", "AACD", "AACD", "AA.." },
    { "duplicate-reverse0-anchorA", "cut1-passive", "A.CD", "A.CD", "A..." },
    { "duplicate-reverse0-anchorA", "cut1-probe-A", "A.CD", "A.CD", "...." },
    { "duplicate-reverse0-anchorA", "cut1-probe-B", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorA", "cut1-probe-C", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorA", "cut1-probe-D", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorA", "cut2-before", "ABCD", "ABCD", "A..." },
    { "duplicate-reverse0-anchorA", "cut2-passive", "ABCD", "ABCD", "A..." },
    { "duplicate-reverse0-anchorA", "cut2-probe-A", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorA", "cut2-probe-B", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorA", "cut2-probe-C", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorA", "cut2-probe-D", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorB", "isolated", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorB", "joined", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorB", "joined-probe-A", "AACD", "AACD", "...." },
    { "duplicate-reverse0-anchorB", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorB", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorB", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorB", "cut0-before", "BBCD", "BBCD", "BB.." },
    { "duplicate-reverse0-anchorB", "cut0-passive", "BBCD", "BBCD", "BB.." },
    { "duplicate-reverse0-anchorB", "cut0-probe-A", "AACD", "AACD", "...." },
    { "duplicate-reverse0-anchorB", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorB", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorB", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "duplicate-reverse0-anchorB", "cut1-before", "BBCD", "BBCD", "BB.." },
    { "duplicate-reverse0-anchorB", "cut1-passive", ".BCD", ".BCD", ".B.." },
    { "duplicate-reverse0-anchorB", "cut1-probe-A", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorB", "cut1-probe-B", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorB", "cut1-probe-C", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorB", "cut1-probe-D", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorB", "cut2-before", "ABCD", "ABCD", ".B.." },
    { "duplicate-reverse0-anchorB", "cut2-passive", "ABCD", "ABCD", ".B.." },
    { "duplicate-reverse0-anchorB", "cut2-probe-A", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorB", "cut2-probe-B", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorB", "cut2-probe-C", "ABCD", "ABCD", "...." },
    { "duplicate-reverse0-anchorB", "cut2-probe-D", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorA", "joined", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorA", "joined-probe-A", "AACD", "AACD", "...." },
    { "duplicate-reverse1-anchorA", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorA", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorA", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorA", "cut0-before", "AACD", "AACD", "AA.." },
    { "duplicate-reverse1-anchorA", "cut0-passive", "AACD", "AACD", "AA.." },
    { "duplicate-reverse1-anchorA", "cut0-probe-A", "AACD", "AACD", "...." },
    { "duplicate-reverse1-anchorA", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorA", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorA", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorA", "cut1-before", "AACD", "AACD", "AA.." },
    { "duplicate-reverse1-anchorA", "cut1-passive", "A.CD", "A.CD", "A..." },
    { "duplicate-reverse1-anchorA", "cut1-probe-A", "A.CD", "A.CD", "...." },
    { "duplicate-reverse1-anchorA", "cut1-probe-B", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorA", "cut1-probe-C", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorA", "cut1-probe-D", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorA", "cut2-before", "ABCD", "ABCD", "A..." },
    { "duplicate-reverse1-anchorA", "cut2-passive", "ABCD", "ABCD", "A..." },
    { "duplicate-reverse1-anchorA", "cut2-probe-A", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorA", "cut2-probe-B", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorA", "cut2-probe-C", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorA", "cut2-probe-D", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorB", "isolated", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorB", "joined", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorB", "joined-probe-A", "AACD", "AACD", "...." },
    { "duplicate-reverse1-anchorB", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorB", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorB", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorB", "cut0-before", "BBCD", "BBCD", "BB.." },
    { "duplicate-reverse1-anchorB", "cut0-passive", "BBCD", "BBCD", "BB.." },
    { "duplicate-reverse1-anchorB", "cut0-probe-A", "AACD", "AACD", "...." },
    { "duplicate-reverse1-anchorB", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorB", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorB", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "duplicate-reverse1-anchorB", "cut1-before", "BBCD", "BBCD", "BB.." },
    { "duplicate-reverse1-anchorB", "cut1-passive", ".BCD", ".BCD", ".B.." },
    { "duplicate-reverse1-anchorB", "cut1-probe-A", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorB", "cut1-probe-B", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorB", "cut1-probe-C", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorB", "cut1-probe-D", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorB", "cut2-before", "ABCD", "ABCD", ".B.." },
    { "duplicate-reverse1-anchorB", "cut2-passive", "ABCD", "ABCD", ".B.." },
    { "duplicate-reverse1-anchorB", "cut2-probe-A", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorB", "cut2-probe-B", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorB", "cut2-probe-C", "ABCD", "ABCD", "...." },
    { "duplicate-reverse1-anchorB", "cut2-probe-D", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse0-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse0-anchorA", "joined", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorA", "joined-probe-A", "AACD", "AACD", "...." },
    { "opposite-edges-reverse0-anchorA", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorA", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorA", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorA", "cut0-before", "AACD", "AACD", "AA.." },
    { "opposite-edges-reverse0-anchorA", "cut0-passive", "AACD", "AACD", "AA.." },
    { "opposite-edges-reverse0-anchorA", "cut0-probe-A", "AACD", "AACD", "...." },
    { "opposite-edges-reverse0-anchorA", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorA", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorA", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorA", "cut1-before", "AACD", "AACD", "AA.." },
    { "opposite-edges-reverse0-anchorA", "cut1-passive", "A.CD", "A.CD", "A..." },
    { "opposite-edges-reverse0-anchorA", "cut1-probe-A", "A.CD", "A.CD", "...." },
    { "opposite-edges-reverse0-anchorA", "cut1-probe-B", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse0-anchorA", "cut1-probe-C", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse0-anchorA", "cut1-probe-D", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse0-anchorB", "isolated", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse0-anchorB", "joined", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorB", "joined-probe-A", "AACD", "AACD", "...." },
    { "opposite-edges-reverse0-anchorB", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorB", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorB", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorB", "cut0-before", "BBCD", "BBCD", "BB.." },
    { "opposite-edges-reverse0-anchorB", "cut0-passive", "BBCD", "BBCD", "BB.." },
    { "opposite-edges-reverse0-anchorB", "cut0-probe-A", "AACD", "AACD", "...." },
    { "opposite-edges-reverse0-anchorB", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorB", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorB", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse0-anchorB", "cut1-before", "BBCD", "BBCD", "BB.." },
    { "opposite-edges-reverse0-anchorB", "cut1-passive", ".BCD", ".BCD", ".B.." },
    { "opposite-edges-reverse0-anchorB", "cut1-probe-A", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse0-anchorB", "cut1-probe-B", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse0-anchorB", "cut1-probe-C", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse0-anchorB", "cut1-probe-D", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse1-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse1-anchorA", "joined", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorA", "joined-probe-A", "AACD", "AACD", "...." },
    { "opposite-edges-reverse1-anchorA", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorA", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorA", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorA", "cut0-before", "AACD", "AACD", "AA.." },
    { "opposite-edges-reverse1-anchorA", "cut0-passive", "AACD", "AACD", "AA.." },
    { "opposite-edges-reverse1-anchorA", "cut0-probe-A", "AACD", "AACD", "...." },
    { "opposite-edges-reverse1-anchorA", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorA", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorA", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorA", "cut1-before", "AACD", "AACD", "AA.." },
    { "opposite-edges-reverse1-anchorA", "cut1-passive", "A.CD", "A.CD", "A..." },
    { "opposite-edges-reverse1-anchorA", "cut1-probe-A", "A.CD", "A.CD", "...." },
    { "opposite-edges-reverse1-anchorA", "cut1-probe-B", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse1-anchorA", "cut1-probe-C", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse1-anchorA", "cut1-probe-D", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse1-anchorB", "isolated", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse1-anchorB", "joined", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorB", "joined-probe-A", "AACD", "AACD", "...." },
    { "opposite-edges-reverse1-anchorB", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorB", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorB", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorB", "cut0-before", "BBCD", "BBCD", "BB.." },
    { "opposite-edges-reverse1-anchorB", "cut0-passive", "BBCD", "BBCD", "BB.." },
    { "opposite-edges-reverse1-anchorB", "cut0-probe-A", "AACD", "AACD", "...." },
    { "opposite-edges-reverse1-anchorB", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorB", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorB", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "opposite-edges-reverse1-anchorB", "cut1-before", "BBCD", "BBCD", "BB.." },
    { "opposite-edges-reverse1-anchorB", "cut1-passive", ".BCD", ".BCD", ".B.." },
    { "opposite-edges-reverse1-anchorB", "cut1-probe-A", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse1-anchorB", "cut1-probe-B", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse1-anchorB", "cut1-probe-C", "ABCD", "ABCD", "...." },
    { "opposite-edges-reverse1-anchorB", "cut1-probe-D", "ABCD", "ABCD", "...." },
    { "connected-nonedge-reverse0-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "connected-nonedge-reverse0-anchorA", "joined", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse0-anchorA", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "connected-nonedge-reverse0-anchorA", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "connected-nonedge-reverse0-anchorA", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse0-anchorA", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse0-anchorA", "cut0-before", "AAAD", "AAAD", "AAA." },
    { "connected-nonedge-reverse0-anchorA", "cut0-passive", "AAAD", "AAAD", "AAA." },
    { "connected-nonedge-reverse0-anchorA", "cut0-probe-A", "AAAD", "AAAD", "...." },
    { "connected-nonedge-reverse0-anchorA", "cut0-probe-B", "BBBD", "BBBD", "...." },
    { "connected-nonedge-reverse0-anchorA", "cut0-probe-C", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse0-anchorA", "cut0-probe-D", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse0-anchorC", "isolated", "ABCD", "ABCD", "...." },
    { "connected-nonedge-reverse0-anchorC", "joined", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse0-anchorC", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "connected-nonedge-reverse0-anchorC", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "connected-nonedge-reverse0-anchorC", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse0-anchorC", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse0-anchorC", "cut0-before", "CCCD", "CCCD", "CCC." },
    { "connected-nonedge-reverse0-anchorC", "cut0-passive", "CCCD", "CCCD", "CCC." },
    { "connected-nonedge-reverse0-anchorC", "cut0-probe-A", "AAAD", "AAAD", "...." },
    { "connected-nonedge-reverse0-anchorC", "cut0-probe-B", "BBBD", "BBBD", "...." },
    { "connected-nonedge-reverse0-anchorC", "cut0-probe-C", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse0-anchorC", "cut0-probe-D", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse1-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "connected-nonedge-reverse1-anchorA", "joined", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse1-anchorA", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "connected-nonedge-reverse1-anchorA", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "connected-nonedge-reverse1-anchorA", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse1-anchorA", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse1-anchorA", "cut0-before", "AAAD", "AAAD", "AAA." },
    { "connected-nonedge-reverse1-anchorA", "cut0-passive", "AAAD", "AAAD", "AAA." },
    { "connected-nonedge-reverse1-anchorA", "cut0-probe-A", "AAAD", "AAAD", "...." },
    { "connected-nonedge-reverse1-anchorA", "cut0-probe-B", "BBBD", "BBBD", "...." },
    { "connected-nonedge-reverse1-anchorA", "cut0-probe-C", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse1-anchorA", "cut0-probe-D", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse1-anchorC", "isolated", "ABCD", "ABCD", "...." },
    { "connected-nonedge-reverse1-anchorC", "joined", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse1-anchorC", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "connected-nonedge-reverse1-anchorC", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "connected-nonedge-reverse1-anchorC", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse1-anchorC", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse1-anchorC", "cut0-before", "CCCD", "CCCD", "CCC." },
    { "connected-nonedge-reverse1-anchorC", "cut0-passive", "CCCD", "CCCD", "CCC." },
    { "connected-nonedge-reverse1-anchorC", "cut0-probe-A", "AAAD", "AAAD", "...." },
    { "connected-nonedge-reverse1-anchorC", "cut0-probe-B", "BBBD", "BBBD", "...." },
    { "connected-nonedge-reverse1-anchorC", "cut0-probe-C", "CCCD", "CCCD", "...." },
    { "connected-nonedge-reverse1-anchorC", "cut0-probe-D", "CCCD", "CCCD", "...." },
    { "self-detach-reverse0-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "self-detach-reverse0-anchorA", "joined", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorA", "joined-probe-A", "AACD", "AACD", "...." },
    { "self-detach-reverse0-anchorA", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorA", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorA", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorA", "cut0-before", "AACD", "AACD", "AA.." },
    { "self-detach-reverse0-anchorA", "cut0-passive", "AACD", "AACD", "AA.." },
    { "self-detach-reverse0-anchorA", "cut0-probe-A", "AACD", "AACD", "...." },
    { "self-detach-reverse0-anchorA", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorA", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorA", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorB", "isolated", "ABCD", "ABCD", "...." },
    { "self-detach-reverse0-anchorB", "joined", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorB", "joined-probe-A", "AACD", "AACD", "...." },
    { "self-detach-reverse0-anchorB", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorB", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorB", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorB", "cut0-before", "BBCD", "BBCD", "BB.." },
    { "self-detach-reverse0-anchorB", "cut0-passive", "BBCD", "BBCD", "BB.." },
    { "self-detach-reverse0-anchorB", "cut0-probe-A", "AACD", "AACD", "...." },
    { "self-detach-reverse0-anchorB", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorB", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "self-detach-reverse0-anchorB", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "self-detach-reverse1-anchorA", "joined", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorA", "joined-probe-A", "AACD", "AACD", "...." },
    { "self-detach-reverse1-anchorA", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorA", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorA", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorA", "cut0-before", "AACD", "AACD", "AA.." },
    { "self-detach-reverse1-anchorA", "cut0-passive", "AACD", "AACD", "AA.." },
    { "self-detach-reverse1-anchorA", "cut0-probe-A", "AACD", "AACD", "...." },
    { "self-detach-reverse1-anchorA", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorA", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorA", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorB", "isolated", "ABCD", "ABCD", "...." },
    { "self-detach-reverse1-anchorB", "joined", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorB", "joined-probe-A", "AACD", "AACD", "...." },
    { "self-detach-reverse1-anchorB", "joined-probe-B", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorB", "joined-probe-C", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorB", "joined-probe-D", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorB", "cut0-before", "BBCD", "BBCD", "BB.." },
    { "self-detach-reverse1-anchorB", "cut0-passive", "BBCD", "BBCD", "BB.." },
    { "self-detach-reverse1-anchorB", "cut0-probe-A", "AACD", "AACD", "...." },
    { "self-detach-reverse1-anchorB", "cut0-probe-B", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorB", "cut0-probe-C", "BBCD", "BBCD", "...." },
    { "self-detach-reverse1-anchorB", "cut0-probe-D", "BBCD", "BBCD", "...." },
    { "exit-neighbor-reverse0-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "exit-neighbor-reverse0-anchorA", "joined", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse0-anchorA", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "exit-neighbor-reverse0-anchorA", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "exit-neighbor-reverse0-anchorA", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse0-anchorA", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse0-anchorA", "exit-before", "AAAD", "AAAD", "...." },
    { "exit-neighbor-reverse0-anchorA", "exit-passive", "AA-D", "AA-D", "..-." },
    { "exit-neighbor-reverse0-anchorA", "exit-probe-A", "AA-D", "AA-D", "..-." },
    { "exit-neighbor-reverse0-anchorA", "exit-probe-B", "BB-D", "BB-D", "..-." },
    { "exit-neighbor-reverse0-anchorA", "exit-probe-D", "BB-D", "BB-D", "..-." },
    { "exit-neighbor-reverse0-anchorC", "isolated", "ABCD", "ABCD", "...." },
    { "exit-neighbor-reverse0-anchorC", "joined", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse0-anchorC", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "exit-neighbor-reverse0-anchorC", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "exit-neighbor-reverse0-anchorC", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse0-anchorC", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse0-anchorC", "exit-before", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse0-anchorC", "exit-passive", "BB-D", "BB-D", "..-." },
    { "exit-neighbor-reverse0-anchorC", "exit-probe-A", "AA-D", "AA-D", "..-." },
    { "exit-neighbor-reverse0-anchorC", "exit-probe-B", "BB-D", "BB-D", "..-." },
    { "exit-neighbor-reverse0-anchorC", "exit-probe-D", "BB-D", "BB-D", "..-." },
    { "exit-neighbor-reverse1-anchorA", "isolated", "ABCD", "ABCD", "...." },
    { "exit-neighbor-reverse1-anchorA", "joined", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse1-anchorA", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "exit-neighbor-reverse1-anchorA", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "exit-neighbor-reverse1-anchorA", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse1-anchorA", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse1-anchorA", "exit-before", "AAAD", "AAAD", "...." },
    { "exit-neighbor-reverse1-anchorA", "exit-passive", "AA-D", "AA-D", "..-." },
    { "exit-neighbor-reverse1-anchorA", "exit-probe-A", "AA-D", "AA-D", "..-." },
    { "exit-neighbor-reverse1-anchorA", "exit-probe-B", "BB-D", "BB-D", "..-." },
    { "exit-neighbor-reverse1-anchorA", "exit-probe-D", "BB-D", "BB-D", "..-." },
    { "exit-neighbor-reverse1-anchorC", "isolated", "ABCD", "ABCD", "...." },
    { "exit-neighbor-reverse1-anchorC", "joined", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse1-anchorC", "joined-probe-A", "AAAD", "AAAD", "...." },
    { "exit-neighbor-reverse1-anchorC", "joined-probe-B", "BBBD", "BBBD", "...." },
    { "exit-neighbor-reverse1-anchorC", "joined-probe-C", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse1-anchorC", "joined-probe-D", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse1-anchorC", "exit-before", "CCCD", "CCCD", "...." },
    { "exit-neighbor-reverse1-anchorC", "exit-passive", "BB-D", "BB-D", "..-." },
    { "exit-neighbor-reverse1-anchorC", "exit-probe-A", "AA-D", "AA-D", "..-." },
    { "exit-neighbor-reverse1-anchorC", "exit-probe-B", "BB-D", "BB-D", "..-." },
    { "exit-neighbor-reverse1-anchorC", "exit-probe-D", "BB-D", "BB-D", "..-." },
    { "children-late0-unparent0", "one-child", "ABAD", "aBaD", "...." },
    { "children-late0-unparent0", "two-children", "AAAD", "aaaD", "...." },
    { "children-late0-unparent0", "destroy-passive", "A.AD", "a.aD", "...." },
    { "children-late0-unparent0", "destroy-refocus", "A.AD", "a.aD", "...." },
    { "children-late0-unparent1", "one-child", "ABAD", "aBaD", "...." },
    { "children-late0-unparent1", "two-children", "AAAD", "aaaD", "...." },
    { "children-late0-unparent1", "unparent-passive", "AAAD", "aaaD", "...." },
    { "children-late0-unparent1", "unparent-refocus", "AAAD", "aaaD", "...." },
    { "children-late0-unparent1", "destroy-passive", "AAAD", "aaaD", "...." },
    { "children-late0-unparent1", "destroy-refocus", "AAAD", "aaaD", "...." },
    { "children-late1-unparent0", "one-child", "CBCD", "CBCD", "...." },
    { "children-late1-unparent0", "two-children", "AAAD", "aaaD", "...." },
    { "children-late1-unparent0", "destroy-passive", "A.AD", "a.aD", "...." },
    { "children-late1-unparent0", "destroy-refocus", "A.AD", "a.aD", "...." },
    { "children-late1-unparent1", "one-child", "CBCD", "CBCD", "...." },
    { "children-late1-unparent1", "two-children", "AAAD", "aaaD", "...." },
    { "children-late1-unparent1", "unparent-passive", "AAAD", "aaaD", "...." },
    { "children-late1-unparent1", "unparent-refocus", "AAAD", "aaaD", "...." },
    { "children-late1-unparent1", "destroy-passive", "AAAD", "aaaD", "...." },
    { "children-late1-unparent1", "destroy-refocus", "AAAD", "aaaD", "...." },
};

struct expected_detach
{
    const char *scenario;
    unsigned int call;
    BOOL result;
    DWORD error;
};
static const struct expected_detach expected_detaches[] =
{
    { "isolated-nonedge-reverse0-anchorA", 0, 0, 0x00000057 },
    { "isolated-nonedge-reverse0-anchorB", 0, 0, 0x00000057 },
    { "isolated-nonedge-reverse1-anchorA", 0, 0, 0x00000057 },
    { "isolated-nonedge-reverse1-anchorB", 0, 0, 0x00000057 },
    { "pair-reverse0-anchorA", 0, 1, 0xdeadbeef },
    { "pair-reverse0-anchorB", 0, 1, 0xdeadbeef },
    { "pair-reverse1-anchorA", 0, 1, 0xdeadbeef },
    { "pair-reverse1-anchorB", 0, 1, 0xdeadbeef },
    { "chain-reverse0-anchorA", 0, 1, 0xdeadbeef },
    { "chain-reverse0-anchorC", 0, 1, 0xdeadbeef },
    { "chain-reverse1-anchorA", 0, 1, 0xdeadbeef },
    { "chain-reverse1-anchorC", 0, 1, 0xdeadbeef },
    { "bridge-reverse0-anchorA", 0, 1, 0xdeadbeef },
    { "bridge-reverse0-anchorD", 0, 1, 0xdeadbeef },
    { "bridge-reverse1-anchorA", 0, 1, 0xdeadbeef },
    { "bridge-reverse1-anchorD", 0, 1, 0xdeadbeef },
    { "cycle-reverse0-anchorA", 0, 1, 0xdeadbeef },
    { "cycle-reverse0-anchorA", 1, 1, 0xdeadbeef },
    { "cycle-reverse0-anchorC", 0, 1, 0xdeadbeef },
    { "cycle-reverse0-anchorC", 1, 1, 0xdeadbeef },
    { "cycle-reverse1-anchorA", 0, 1, 0xdeadbeef },
    { "cycle-reverse1-anchorA", 1, 1, 0xdeadbeef },
    { "cycle-reverse1-anchorC", 0, 1, 0xdeadbeef },
    { "cycle-reverse1-anchorC", 1, 1, 0xdeadbeef },
    { "duplicate-reverse0-anchorA", 0, 1, 0xdeadbeef },
    { "duplicate-reverse0-anchorA", 1, 1, 0xdeadbeef },
    { "duplicate-reverse0-anchorA", 2, 0, 0x00000057 },
    { "duplicate-reverse0-anchorB", 0, 1, 0xdeadbeef },
    { "duplicate-reverse0-anchorB", 1, 1, 0xdeadbeef },
    { "duplicate-reverse0-anchorB", 2, 0, 0x00000057 },
    { "duplicate-reverse1-anchorA", 0, 1, 0xdeadbeef },
    { "duplicate-reverse1-anchorA", 1, 1, 0xdeadbeef },
    { "duplicate-reverse1-anchorA", 2, 0, 0x00000057 },
    { "duplicate-reverse1-anchorB", 0, 1, 0xdeadbeef },
    { "duplicate-reverse1-anchorB", 1, 1, 0xdeadbeef },
    { "duplicate-reverse1-anchorB", 2, 0, 0x00000057 },
    { "opposite-edges-reverse0-anchorA", 0, 1, 0xdeadbeef },
    { "opposite-edges-reverse0-anchorA", 1, 1, 0xdeadbeef },
    { "opposite-edges-reverse0-anchorB", 0, 1, 0xdeadbeef },
    { "opposite-edges-reverse0-anchorB", 1, 1, 0xdeadbeef },
    { "opposite-edges-reverse1-anchorA", 0, 1, 0xdeadbeef },
    { "opposite-edges-reverse1-anchorA", 1, 1, 0xdeadbeef },
    { "opposite-edges-reverse1-anchorB", 0, 1, 0xdeadbeef },
    { "opposite-edges-reverse1-anchorB", 1, 1, 0xdeadbeef },
    { "connected-nonedge-reverse0-anchorA", 0, 0, 0x00000057 },
    { "connected-nonedge-reverse0-anchorC", 0, 0, 0x00000057 },
    { "connected-nonedge-reverse1-anchorA", 0, 0, 0x00000057 },
    { "connected-nonedge-reverse1-anchorC", 0, 0, 0x00000057 },
    { "self-detach-reverse0-anchorA", 0, 0, 0x00000057 },
    { "self-detach-reverse0-anchorB", 0, 0, 0x00000057 },
    { "self-detach-reverse1-anchorA", 0, 0, 0x00000057 },
    { "self-detach-reverse1-anchorB", 0, 0, 0x00000057 },
};

static void attachment_error(const char *operation, DWORD error)
{
    InterlockedIncrement(&attachment_errors);
    ok(0, "%s failed, error %lu\n", operation, error);
}

static HWND expected_window(char c)
{
    if (c == '.') return NULL;
    if (c == 'a') return workers[2].children[0];
    if (c == 'b') return workers[2].children[1];
    if (c >= 'A' && c <= 'D') return workers[c - 'A'].window;
    return NULL;
}

static void snapshot(const char *stage)
{
    const struct expected_state *expect = NULL;
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(expected_states); ++i)
        if (!strcmp(expected_states[i].scenario, scenario) && !strcmp(expected_states[i].stage, stage))
        { expect = &expected_states[i]; break; }
    ok(expect != NULL, "%s: no expectation for %s\n", scenario, stage);
    if (!expect) return;
    for (i = 0; i < THREADS; ++i)
    {
        GUITHREADINFO info;
        BOOL ret, missing_destroy_split;
        if (!workers[i].thread) continue;
        winetest_push_context("%s %s observer %c", scenario, stage, 'A' + i);
        memset(&info, 0, sizeof(info));
        info.cbSize = sizeof(info);
        ret = GetGUIThreadInfo(workers[i].tid, &info);
        ok(ret, "GetGUIThreadInfo failed, error %lu\n", GetLastError());
        if (ret)
        {
            /* Direct child destruction's missing implicit detach is separate
             * from the component split. The surviving host/C state is ordinary. */
            missing_destroy_split = i == 1 && strstr(scenario, "-unparent0") &&
                                    !strncmp(stage, "destroy-", 8);
            todo_wine_if(missing_destroy_split)
            ok(info.hwndActive == expected_window(expect->active[i]), "active %p, expected %p\n",
               info.hwndActive, expected_window(expect->active[i]));
            todo_wine_if(missing_destroy_split)
            ok(info.hwndFocus == expected_window(expect->focus[i]), "focus %p, expected %p\n",
               info.hwndFocus, expected_window(expect->focus[i]));
            ok(info.hwndCapture == expected_window(expect->capture[i]), "capture %p, expected %p\n",
               info.hwndCapture, expected_window(expect->capture[i]));
        }
        winetest_pop_context();
    }
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    struct worker *w = (struct worker *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    HWND previous = NULL;
    DWORD error;
    BOOL ret = TRUE;
    if (message == WM_NCCREATE)
    {
        const CREATESTRUCTW *cs = (const CREATESTRUCTW *)lp;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
    }
    if (message != COMMAND) return DefWindowProcW(hwnd, message, wp, lp);
    SetLastError(0xdeadbeef);
    switch (wp)
    {
    case FOCUS:
        previous = SetFocus(hwnd);
        error = GetLastError();
        ret = GetFocus() == hwnd;
        break;
    case ACTIVATE:
        SetForegroundWindow(hwnd);
        error = GetLastError();
        SetLastError(0xdeadbeef);
        previous = SetActiveWindow(hwnd);
        error = GetLastError();
        SetLastError(0xdeadbeef);
        previous = SetFocus(hwnd);
        error = GetLastError();
        ret = GetActiveWindow() == hwnd && GetFocus() == hwnd;
        break;
    case CAPTURE:
        previous = SetCapture(hwnd);
        error = GetLastError();
        ret = GetCapture() == hwnd;
        break;
    case RELEASE:
        ret = ReleaseCapture();
        error = GetLastError();
        break;
    case CHILD_CREATE:
        if (lp < 0 || lp > 1 || w->index != 2) return FALSE;
        w->children[lp] = CreateWindowExW(0, L"WinumAttachmentMeasure", L"child",
                WS_CHILD | WS_VISIBLE, 5, 5, 50, 30, workers[lp].window,
                NULL, GetModuleHandleW(NULL), NULL);
        error = GetLastError();
        ret = w->children[lp] != NULL;
        break;
    case CHILD_FOCUS:
        previous = SetFocus(w->children[0]);
        error = GetLastError();
        ret = GetFocus() == w->children[0];
        break;
    case CHILD_UNPARENT:
        previous = SetParent(w->children[1], NULL);
        error = GetLastError();
        ret = previous != NULL;
        break;
    case CHILD_DESTROY:
        ret = DestroyWindow(w->children[1]);
        error = GetLastError();
        w->children[1] = NULL;
        break;
    case STOP: PostQuitMessage(0); return TRUE;
    default: return FALSE;
    }
    if (!ret && (wp == CHILD_CREATE || wp == CHILD_UNPARENT || wp == CHILD_DESTROY))
        attachment_error("OwnChildOperation", error);
    return ret;
}

static DWORD WINAPI worker_main(void *arg)
{
    struct worker *w = arg;
    MSG msg;
    int ret;
    w->window = CreateWindowExW(0, L"WinumAttachmentMeasure", L"Input attachment MEASURE",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, 20 + 130 * w->index, 30,
            120, 140, NULL, NULL, GetModuleHandleW(NULL), w);
    if (!w->window) attachment_error("CreateOwnWindow", GetLastError());
    if (!SetEvent(w->ready)) attachment_error("SignalReady", GetLastError());
    if (!w->window) return 1;
    while ((ret = GetMessageW(&msg, NULL, 0, 0)) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (ret < 0) attachment_error("GetMessageW", GetLastError());
    if (w->children[1]) DestroyWindow(w->children[1]);
    if (w->children[0]) DestroyWindow(w->children[0]);
    DestroyWindow(w->window);
    return 0;
}

static BOOL command(unsigned int worker, enum operation op, LPARAM arg)
{
    DWORD_PTR result;
    if (!SendMessageTimeoutW(workers[worker].window, COMMAND, op, arg,
                             SMTO_ABORTIFHUNG, 5000, &result))
    {
        attachment_error("SendOwnCommand", GetLastError());
        return FALSE;
    }
    /* A NULL previous focus/capture is not a failed operation. Only the
     * fixture's own create/unparent/destroy results are prerequisites;
     * focus/activation/capture outcomes themselves are measurements. */
    if ((op == CHILD_CREATE || op == CHILD_UNPARENT || op == CHILD_DESTROY) && !result)
        return FALSE;
    return TRUE;
}

static void stop_worker(unsigned int i)
{
    if (!workers[i].thread) return;
    if (WaitForSingleObject(workers[i].thread, 0) == WAIT_TIMEOUT)
        command(i, STOP, 0);
    if (WaitForSingleObject(workers[i].thread, 7000) != WAIT_OBJECT_0)
    {
        /* Do not terminate an individual thread holding USER locks. */
        attachment_error("JoinOwnWorker", ERROR_TIMEOUT);
        TerminateProcess(GetCurrentProcess(), 2);
    }
    CloseHandle(workers[i].thread);
    CloseHandle(workers[i].ready);
    workers[i].thread = workers[i].ready = NULL;
    workers[i].window = NULL;
}

static BOOL start_workers(void)
{
    unsigned int i;
    memset(workers, 0, sizeof(workers));
    for (i = 0; i < THREADS; ++i)
    {
        workers[i].index = i;
        workers[i].ready = CreateEventW(NULL, TRUE, FALSE, NULL);
        if (!workers[i].ready) { attachment_error("CreateReady", GetLastError()); return FALSE; }
        workers[i].thread = CreateThread(NULL, 0, worker_main, &workers[i], 0, &workers[i].tid);
        if (!workers[i].thread)
        {
            attachment_error("CreateOwnWorker", GetLastError());
            CloseHandle(workers[i].ready);
            workers[i].ready = NULL;
            return FALSE;
        }
        if (WaitForSingleObject(workers[i].ready, 7000) != WAIT_OBJECT_0 || !workers[i].window)
        { attachment_error("WaitOwnWorker", ERROR_TIMEOUT); return FALSE; }
    }
    return TRUE;
}

static BOOL edge_call(unsigned int from, unsigned int to, BOOL attach)
{
    const struct expected_detach *expect = NULL;
    unsigned int i;
    DWORD error;
    BOOL ret;
    SetLastError(0xdeadbeef);
    ret = AttachThreadInput(workers[from].tid, workers[to].tid, attach);
    error = GetLastError();
    if (attach)
    {
        ok(ret, "%s: attach %c-%c failed, error %lu\n", scenario, 'A' + from, 'A' + to, error);
        return ret;
    }
    for (i = 0; i < ARRAY_SIZE(expected_detaches); ++i)
        if (!strcmp(expected_detaches[i].scenario, scenario) && expected_detaches[i].call == detach_call)
        { expect = &expected_detaches[i]; break; }
    ok(expect != NULL, "%s: missing detach expectation %u\n", scenario, detach_call);
    ++detach_call;
    if (expect)
    {
        ok(ret == expect->result, "%s: detach %c-%c returned %d, expected %d\n",
           scenario, 'A' + from, 'A' + to, ret, expect->result);
        if (!ret)
        {
            /* Wine rejects self-detach with access denied instead of invalid parameter. */
            todo_wine_if(from == to)
            ok(error == expect->error, "%s: got error %lu, expected %lu\n", scenario, error, expect->error);
        }
    }
    return ret;
}

static BOOL probe_focus(const char *stage)
{
    unsigned int i;
    char label[64];
    for (i = 0; i < THREADS; ++i)
    {
        if (!workers[i].thread) continue;
        if (!command(i, FOCUS, 0)) return FALSE;
        snprintf(label, sizeof(label), "%s-probe-%c", stage, 'A' + i);
        snapshot(label);
    }
    return TRUE;
}

static BOOL graph_case(const struct graph *g, BOOL reverse, unsigned int anchor)
{
    char name[96], stage[40];
    unsigned int i;
    BOOL reached_end = FALSE;
    LONG errors_before = attachment_errors;
    snprintf(name, sizeof(name), "%s-reverse%d-anchor%c", g->name, reverse, 'A' + anchor);
    scenario = name;
    detach_call = 0;
    if (!start_workers()) goto cleanup;
    if (!command(anchor, ACTIVATE, 0)) goto cleanup;
    snapshot("isolated");
    for (i = 0; i < g->count; ++i)
    {
        if (!edge_call(g->edges[i].from, g->edges[i].to, TRUE))
        { attachment_error("EstablishOwnGraph", ERROR_INVALID_DATA); goto cleanup; }
    }
    snapshot("joined");
    if (!probe_focus("joined")) goto cleanup;
    for (i = 0; i < g->cuts; ++i)
    {
        if (!command(anchor, ACTIVATE, 0)) goto cleanup;
        if (!command(anchor, CAPTURE, 0)) goto cleanup;
        snprintf(stage, sizeof(stage), "cut%u-before", i);
        snapshot(stage);
        edge_call(reverse ? g->remove[i].to : g->remove[i].from,
                  reverse ? g->remove[i].from : g->remove[i].to, FALSE);
        snprintf(stage, sizeof(stage), "cut%u-passive", i);
        snapshot(stage);
        if (!command(anchor, RELEASE, 0)) goto cleanup;
        snprintf(stage, sizeof(stage), "cut%u", i);
        if (!probe_focus(stage)) goto cleanup;
    }
    if (g->exit_worker >= 0)
    {
        if (!command(anchor, ACTIVATE, 0)) goto cleanup;
        snapshot("exit-before");
        stop_worker(g->exit_worker);
        snapshot("exit-passive");
        if (!probe_focus("exit")) goto cleanup;
    }
    reached_end = TRUE;
cleanup:
    /* Stopping all own threads also removes their explicit attachments. */
    for (i = THREADS; i-- > 0;) stop_worker(i);
    reached_end = reached_end && attachment_errors == errors_before;

    return reached_end;
}

static BOOL child_case(BOOL late, BOOL unparent)
{
    char name[64];
    unsigned int i;
    BOOL reached_end = FALSE;
    LONG errors_before = attachment_errors;
    snprintf(name, sizeof(name), "children-late%d-unparent%d", late, unparent);
    scenario = name;

    if (!start_workers()) goto cleanup;
    if (!command(0, ACTIVATE, 0)) goto cleanup;
    if (!command(2, CHILD_CREATE, 0)) goto cleanup;
    if (!late && !command(2, CHILD_FOCUS, 0)) goto cleanup;
    snapshot("one-child");
    if (!command(2, CHILD_CREATE, 1)) goto cleanup;
    if (!command(2, CHILD_FOCUS, 0)) goto cleanup;
    snapshot("two-children");
    if (unparent)
    {
        if (!command(2, CHILD_UNPARENT, 0)) goto cleanup;
        snapshot("unparent-passive");
        if (!command(2, CHILD_FOCUS, 0)) goto cleanup;
        snapshot("unparent-refocus");
    }
    if (!command(2, CHILD_DESTROY, 0)) goto cleanup;
    snapshot("destroy-passive");
    if (!command(2, CHILD_FOCUS, 0)) goto cleanup;
    snapshot("destroy-refocus");
    reached_end = TRUE;
cleanup:
    for (i = THREADS; i-- > 0;) stop_worker(i);
    reached_end = reached_end && attachment_errors == errors_before;

    return reached_end;
}

static void test_input_components(void)
{
    WNDCLASSW wc = {0};
    unsigned int i, reverse, late, unparent;
    BOOL ret;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"WinumAttachmentMeasure";
    ret = RegisterClassW(&wc);
    ok(ret, "RegisterClass failed, error %lu\n", GetLastError());
    if (!ret) return;
    for (i = 0; i < ARRAY_SIZE(graphs); ++i)
        for (reverse = 0; reverse < 2; ++reverse)
        {
            completed_scenarios += graph_case(&graphs[i], reverse, 0);
            completed_scenarios += graph_case(&graphs[i], reverse, graphs[i].other_anchor);
        }
    for (late = 0; late < 2; ++late)
        for (unparent = 0; unparent < 2; ++unparent)
            completed_scenarios += child_case(late, unparent);
    ret = UnregisterClassW(wc.lpszClassName, wc.hInstance);
    ok(ret, "UnregisterClass failed, error %lu\n", GetLastError());
    ok(completed_scenarios == 44, "completed %u scenarios\n", completed_scenarios);
}
