#include <muduo/examples/shudu-resolver/resolver.h>
#include <muduo/examples/shudu-resolver/shudu-algorithm/shuduku.h>
#include <muduo/base/logging.h>

static void solve(const tcp_connection::tcp_connection_ptr& conn, const string& puzzle, const string& id) {
    LOG_DEBUG << conn->name();
    string result = solveSudoku(puzzle);
    if (id.empty()) {
        conn->send(result+"\r\n");
    } else {
        conn->send(id+":"+result+"\r\n");
    }
}

using namespace muduo;
using namespace muduo::net;

shudu_resolver::shudu_resolver(event_loop* const loop, const inet_address& serAddr, const int work_thread_num)
    : loop_(loop)
    , work_thread_num_(work_thread_num)
    , server_(loop, serAddr, "shudu-resolver")
    , threadPool_("work-threadPool") {
        server_.set_onConnection_callback([this](const tcp_connection_ptr& conn) { this->onNewConnection(conn); });
        server_.set_onMessage_callback([this](const tcp_connection_ptr& conn, buffer* msg, TimeStamp recv_time) {
            this->onMessage(conn, msg, recv_time);
        });

        // use multiple Reactor:
        // One main Reactor is responsible for accepting and dispatch new connections
        // other sub-Reactor is responsible for handle connection event 
        // server_.set_IO_threadNum(3);   
    }

void shudu_resolver::onNewConnection(const tcp_connection::tcp_connection_ptr& conn) {
    LOG_INFO << "ChargenServer - " << conn->get_peer_addr().ipAndPort() << " -> "
        << conn->get_local_addr().ipAndPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");
}

void shudu_resolver::onMessage(const tcp_connection_ptr& conn, buffer* msg, TimeStamp recv_time) {
    LOG_DEBUG << conn->name();
    std::size_t len = msg->readable_bytes();
    while (len >= static_cast<std::size_t>(kCells+2)) {  // payload + \r\n
        const char* crlf_pos = msg->find_CRLF();
        if (crlf_pos) {
            string payload(msg->peek(), crlf_pos);  // get payload, not including "\r\n"
            msg->retrieve_until(crlf_pos+2);
            len = msg->readable_bytes();            // update len

            bool ok = process_request(conn, payload);   // check payload
            if (!ok) {
                conn->send("Bad request!\r\n");
                conn->shutdown();
                break;
            }
        } else if (len > 100) {   // "id" + ":" + "payload[81 Bytes]" + "\r\n" 
            conn->send("ID too long!\r\n");
            conn->shutdown();
            break;
        } else {
            break;
        }
    }
}

bool shudu_resolver::process_request(const tcp_connection_ptr& conn, const string& payload) {
    string id;
    string puzzle;
    bool goodRequest = true;

    string::const_iterator id_end_pos = std::find(payload.begin(), payload.end(), ':');
    if (id_end_pos != payload.end()) {  // including ID field
        id.assign(payload.begin(), id_end_pos);
        puzzle.assign(id_end_pos+1, payload.end());
    } else {    // not have ID field
        puzzle = payload;
    }

    if (puzzle.size() == implicit_cast<size_t>(kCells)) {
        threadPool_.run([id, puzzle, conn]() { solve(conn, puzzle, id); });
    } else {    // puzzle data not enough
        goodRequest = false;
    }
    return goodRequest;
}
