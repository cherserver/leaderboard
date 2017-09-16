#include <iostream>
#include <map>
#include <list>
#include <SimpleAmqpClient/SimpleAmqpClient.h>

#include <lb_defines.h>
#include <lb_error.h>
#include <lb_functions.h>

using namespace std;
using namespace AmqpClient;

void Debug(const string &msg) {
	cout << msg << endl;
}

struct BoardDesc;

typedef list<BoardDesc> BoardList;

struct UserDesc {
	string name;
	BoardList::iterator board;
};

typedef map<int64_t, UserDesc> UserMap;

struct BoardDesc {
	double amount;
	int64_t place;
	UserMap::iterator user;
	BoardDesc()
	: amount(0), place(1) {}
};

class LeaderBoard {
public:
	LeaderBoard()
	: m_week_begin(date::GetWeekBegin())
	, m_week_end(date::GetWeekEnd()) {}

	bool HasUser(const int64_t id) const {
		return m_users.find(id) != m_users.end();
	}

	void AssertUser(const int64_t id) const {
		if (!HasUser(id))
			throw err::Error("missed", "user_id", str::Str(id));
	}

	string GetStatMessage(const int64_t id) {
		CheckWeeklyDrop();
		return "";
	}

	void AddUser(const int64_t id, const string &name) {
		UserDesc udesc;
		udesc.name = name;
		auto inserted = m_users.insert(std::make_pair(id, udesc));
		if (!inserted.second)
			throw err::Error("exists", "user_id", str::Str(id));

		int64_t place = 1;
		if (!m_board.empty())
			place = m_board.front().place + 1;
		BoardDesc desc;
		desc.amount = 0;
		desc.place = place;
		desc.user = inserted.first;
		m_board.push_front(desc);
		inserted.first->second.board = m_board.begin();

		DebugContents();
	}

	void RenameUser(const int64_t id, const string &new_name) {
		auto fnd_user = m_users.find(id);
		if (fnd_user == m_users.end())
			throw err::Error("missed", "user_id", str::Str(id));
		fnd_user->second.name = new_name;
	}

	void AddWin(const int64_t id, const date::SystemTimePoint &date, double amount) {
		CheckWeeklyDrop();

		if (date < m_week_begin || date > m_week_end)
			throw err::Error("invalid", "date", date::Format(date));

		auto fnd_user = m_users.find(id);
		if (fnd_user == m_users.end())
			throw err::Error("missed", "user_id", str::Str(id));

		auto cur_board = fnd_user->second.board;
		double new_amount = cur_board->amount + amount;

		auto next_board = cur_board;
		++next_board;

		if (next_board == m_board.end() || next_board->amount > new_amount) {
			cur_board->amount = new_amount;
			DebugContents();
			return;
		}

		int64_t place = 1;
		for ( ; ; ++next_board) {
			if (next_board == m_board.end()) {
				break;
			} else if (next_board->amount >= new_amount) {
				place = next_board->place + 1;
				break;
			}
			++(next_board->place);
		}

		cur_board->amount = new_amount;
		cur_board->place = place;

		fnd_user->second.board = m_board.insert(next_board, *cur_board);
		m_board.erase(cur_board);

		DebugContents();
	}
private:
	date::SystemTimePoint m_week_begin;
	date::SystemTimePoint m_week_end;

	UserMap m_users;
	BoardList m_board;

	void CheckWeeklyDrop() {
		if (chrono::system_clock::now() <= m_week_end)
			return;

		for (auto &elem : m_board) {
			elem.amount = 0;
		}

		m_week_begin = date::GetWeekBegin();
		m_week_end = date::GetWeekEnd();
	}

	void DebugContents() const {
		Debug("Board contents:");

		for (auto cur = m_board.rbegin(); cur != m_board.rend(); ++cur)
			Debug("\t" + str::Str(cur->place) + ". " + cur->user->second.name + "  " + str::Str(cur->amount, 2));
	}
} leaderboard;

class Reminder {
public:
	void ConnectUser(const int64_t id) {

	}

	void DisconnectUser(const int64_t id) {

	}
private:
} reminder;

class IncomingListener {
public:
	void Start() {
		m_connection = Channel::Create("localhost");
		m_connection->DeclareQueue(LB_INPUT_QUEUE, false, false, false, false);
		m_consumer = m_connection->BasicConsume(LB_INPUT_QUEUE, "", true, false);

		Debug("Leaderbord started");
		while(true)
			ProcessRequest();
	}
private:
	Channel::ptr_t m_connection;
	string m_consumer;

	void ProcessRequest() {
		Envelope::ptr_t env = m_connection->BasicConsumeMessage(m_consumer);

		string msg = env->Message()->Body();
		ProcessMessage(msg);

		m_connection->BasicAck(env);
	}

	void ProcessMessage(string &msg) const {
		//Debug(msg);
		string msg_type = str::GetWord(msg, '\n');
		Debug("Incoming message: '" + msg_type + "'");
		string id_str = str::GetWord(msg, '\n');

		//валидировать id в реальности надо после валидации типа сообщения
		if (!test::Numeric(id_str))
			throw err::Error("invalid", "id", id_str);

		auto id = str::Int64(id_str);

		try {
			if (msg_type == MSG_USER_REGISTER) {
				string name = str::GetWord(msg, '\n');
				if (!test::Username(name))
					throw err::Error("invalid", "username", name);

				leaderboard.AddUser(id, name);
			} else if (msg_type == MSG_USER_RENAME) {
				string new_name = str::GetWord(msg, '\n');
				if (!test::Username(new_name))
					throw err::Error("invalid", "username", new_name);

				leaderboard.RenameUser(id, new_name);
			} else if (msg_type == MSG_USER_WON) {
				string date_str = str::GetWord(msg, '\n');
				string amount_str = str::GetWord(msg, '\n'); //leave unvalidated
				auto amount = str::Double(amount_str);
				if (amount <= 0)
					throw err::Error("invalid", "amount", amount_str);

				leaderboard.AddWin(id, date::FromString(date_str), amount);
			} else if (msg_type == MSG_USER_CONNECT) {
				leaderboard.AssertUser(id);
				reminder.ConnectUser(id);
			} else if (msg_type == MSG_USER_DISCONNECT) {
				leaderboard.AssertUser(id);
				reminder.DisconnectUser(id);
			} else
				throw err::Error("invalid", "msg_type", msg_type);
		} catch(const err::Error& e) {
			//Обработка сообщений об ошибках
			Debug(string("Failed to process request: ") + e.what());
		}
	}
};

int main() {
	try {
		IncomingListener().Start();
	} catch (const std::exception &e) {
		cout << "Unexpected error thrown: " << e.what() << endl;
		return EXIT_FAILURE;
	} catch (...) {
		cout << "Unknown unexpected error thrown" << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
