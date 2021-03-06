#include <atomic>
#include <condition_variable>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <SimpleAmqpClient/SimpleAmqpClient.h>

#include <lb_defines.h>
#include <lb_error.h>
#include <lb_functions.h>

using namespace std;
using namespace AmqpClient;

void Debug(const string &msg) {
	cout << msg << endl;
}

/*
 * Класс, занимающийся ведением лидерборда
 * Хранилище пользователей - set
 * Хранилище выигрышей - list (сортированный по убыванию позиции)
 * Элементы обоих хранилищ связаны друг с другом итераторами чтобы исключить лишний поиск
 */
class LeaderBoard {
public:
	LeaderBoard()
	: m_week_begin(date::GetWeekBegin())
	, m_week_end(date::GetWeekEnd()) {}

	bool HasUser(const int64_t id) const {
		lock_guard<recursive_mutex> cs(m_mutex);

		return m_users.find(id) != m_users.end();
	}

	void AssertUser(const int64_t id) const {
		if (!HasUser(id))
			throw err::Error("missed", "user_id", str::Str(id));
	}

	string GetStatMessage(const int64_t id) {
		lock_guard<recursive_mutex> cs(m_mutex);

		CheckWeeklyDrop();

		auto fnd_user = m_users.find(id);
		if (fnd_user == m_users.end())
			throw err::Error("missed", "user_id", str::Str(id));

		//первые 10 позиций рейтинга, позицию юзера в рейтинге, +- 10 соседей по рейтингу для текущего пользователя
		if (m_board.empty())
			throw err::Error("missed", "leaderboard");

		//Формат не ограничен, поэтому выведу в человекочитаемом виде
		string result = "User:";
		result += "\n" + ToString(fnd_user->second.board);

		result += "\nLeaders:";
		int leader_count = MAX_NEIGHBOURS;
		for (auto lead = m_board.rbegin(); lead != m_board.rend(); ++lead) {
			result += "\n" + ToString(lead);

			--leader_count;
			if (leader_count <= 0)
				break;
		}

		result += "\nNeighbours up:";
		auto cur_board_up = fnd_user->second.board;
		++cur_board_up;

		if (cur_board_up == m_board.end()) {
			result += " empty";
		} else {
			int count = MAX_NEIGHBOURS;
			string subresult;
			for (; cur_board_up != m_board.end(); ++cur_board_up) {
				subresult = "\n" + ToString(cur_board_up) + subresult;

				--count;
				if (count <=0)
					break;
			}

			result += subresult;
		}

		result += "\nNeighbours down:";
		auto cur_board_down = fnd_user->second.board;
		if (cur_board_down == m_board.begin()) {
			result += " empty";
		} else {
			int count = MAX_NEIGHBOURS;
			while (count > 0) {
				--cur_board_down;
				result += "\n" + ToString(cur_board_down);
				if (cur_board_down == m_board.begin())
					break;
				--count;
			}
		}

		return result;
	}

	/*
	 * Вызывается при user_registered
	 */
	void AddUser(const int64_t id, const string &name) {
		lock_guard<recursive_mutex> cs(m_mutex);

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

		//DebugContents();
	}

	/*
	 * Вызывается при user_renamed
	 */
	void RenameUser(const int64_t id, const string &new_name) {
		lock_guard<recursive_mutex> cs(m_mutex);

		auto fnd_user = m_users.find(id);
		if (fnd_user == m_users.end())
			throw err::Error("missed", "user_id", str::Str(id));
		fnd_user->second.name = new_name;
	}

	/*
	 * Вызывается при user_deal_won
	 */
	void AddWin(const int64_t id, const date::SystemTimePoint &date, double amount) {
		lock_guard<recursive_mutex> cs(m_mutex);

		CheckWeeklyDrop();

		if (date < m_week_begin || date > m_week_end)
			throw err::Error("not_this_week", "date", date::Format(date));

		auto fnd_user = m_users.find(id);
		if (fnd_user == m_users.end())
			throw err::Error("missed", "user_id", str::Str(id));

		auto cur_board = fnd_user->second.board;
		double new_amount = cur_board->amount + amount;

		auto next_board = cur_board;
		++next_board;

		if (next_board == m_board.end() || next_board->amount > new_amount) {
			cur_board->amount = new_amount;
			//DebugContents();
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

		//DebugContents();
	}
private:
	date::SystemTimePoint m_week_begin;
	date::SystemTimePoint m_week_end;

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

	UserMap m_users;
	BoardList m_board;

	mutable recursive_mutex m_mutex;

	void CheckWeeklyDrop() {
		lock_guard<recursive_mutex> cs(m_mutex);

		if (chrono::system_clock::now() <= m_week_end)
			return;

		for (auto &elem : m_board) {
			elem.amount = 0;
		}

		m_week_begin = date::GetWeekBegin();
		m_week_end = date::GetWeekEnd();
	}

	template <class IteratorType>
	static string ToString(IteratorType &user_pos) {
		return str::Str(user_pos->place) + ". " +
				user_pos->user->second.name +
				" (id:" + str::Str(user_pos->user->first) + ")" +
				"  " + str::Str(user_pos->amount, 2);
	}

	void DebugContents() const {
		Debug("Board contents:");

		for (auto cur = m_board.rbegin(); cur != m_board.rend(); ++cur)
			Debug("\t" + ToString(cur));
	}
} leaderboard;

/*
 * Класс, занимающийся непосредственно рассылкой сообщений - выходной канал связи
 * Обслуживает очередь сообщений, используя для хранения queue
 */
class Producer {
public:
	Producer()
	: m_stopped(false)
	, m_connection(Channel::Create(RABBITMQ_HOST)) {
		m_connection->DeclareQueue(LB_OUTPUT_QUEUE, false, false, false, false);
	}

	void AddMessage(const string &msg) {
		{
			lock_guard<mutex> cs(m_data_mutex);
			m_messages.push(msg);
		}
		m_can_process.notify_one();
	}

	bool HasMessages() {
		lock_guard<mutex> cs(m_data_mutex);
		return !m_messages.empty();
	}

	void SendMessages() {
		while(true) {
			unique_lock<mutex> wait_lock(m_wait_mutex);
			while(!m_stopped && !HasMessages()) {
				m_can_process.wait(wait_lock);
			}

			if (m_stopped)
				return;

			lock_guard<mutex> cs(m_data_mutex);
			if (m_messages.empty())
				continue;

			m_connection->BasicPublish("", LB_OUTPUT_ROUTE, BasicMessage::Create(m_messages.front()));
			m_messages.pop();
		}
	}
	void Stop() {
		unique_lock<mutex> wait_lock(m_wait_mutex);
		m_stopped = true;
		m_can_process.notify_one();
	}
private:
	atomic_bool m_stopped;
	mutex m_data_mutex;
	mutex m_wait_mutex;
	condition_variable m_can_process;
	Channel::ptr_t m_connection;

	queue<string> m_messages;
} producer;

/*
 * Класс, занимающийся планированием рассылки сообщений
 * Содержит список подключенных пользователей (set)
 * и list запланированного времени отправки сообщений
 * Наиболее раннее (ближайшее) запланированное время - внизу списка
 *
 * Элементы списков связаны итераторами для исключения поиска данных
 *
 * Внутри класса также реализован отдельный поток Sleeper
 * для организации прерываемого ожидания запланированного времени
 *
 * Прерывание ожидания вызывается в случае подключения пользователя -
 * его сообщение должно быть отправлено сейчас
 *
 */
class Reminder {
public:
	Reminder()
	: m_stopped(false) {
	}

	~Reminder() {
		if (!m_stopped)
			Stop();
	}

	/*
	 * Вызывается при user_connected
	 */
	void ConnectUser(const int64_t id) {
		lock_guard<mutex> cs(m_data_mutex);

		auto inserted = m_users.insert(std::make_pair(id, m_reminder.end()));
		if (!inserted.second)
			throw err::Error("already connected", "user_id", str::Str(id));

		ReminderDesc desc;
		desc.user = inserted.first;
		m_reminder.push_back(desc);

		//запланируем сейчас и отменим ожидание следующего
		m_sleeper.WakeUp();
	}

	/*
	 * Вызывается при user_disconnected
	 */
	void DisconnectUser(const int64_t id) {
		lock_guard<mutex> cs(m_data_mutex);

		auto fnd_user = m_users.find(id);
		if (fnd_user == m_users.end())
			return;

		m_reminder.erase(fnd_user->second);
		m_users.erase(fnd_user);
	}

	void Process() {
		while(true) {
			if (m_stopped)
				return;

			date::SteadyTimePoint timepoint;
			string message;

			//чтобы не блокировать список юзеров лишнее время. Например во время постановки сообщения в очередь
			{
				lock_guard<mutex> cs(m_data_mutex);
				//DebugContents();
				auto cur_time = std::chrono::steady_clock::now();
				if (m_reminder.empty()) {
					timepoint = cur_time + chrono::minutes(1); //какое-то время - разбудят раньше, если что
				} else {
					const auto &check = m_reminder.back();
					if (check.when <= cur_time) {
						try {
							message = leaderboard.GetStatMessage(check.user->first);
						} catch(const err::Error &e) {
							Debug("Failed to get message. Error: " + string(e.what()));
						}

						m_reminder.push_front(check);

						auto &new_check = m_reminder.front();
						new_check.when = cur_time + chrono::minutes(1);
						new_check.user->second = m_reminder.begin();

						m_reminder.pop_back();

						timepoint = m_reminder.back().when;
					} else {
						timepoint = check.when;
					}
				}
			}

			if (!message.empty())
				producer.AddMessage(message);

			m_sleeper.SleepUntil(timepoint);
		}
	}

	void Stop() {
		m_stopped = true;
		m_sleeper.Stop();
	}
private:
	void DebugContents() const {
		Debug("Reminder contents:");

		for (auto cur = m_reminder.begin(); cur != m_reminder.end(); ++cur)
			Debug("\t" + str::Str(cur->user->first));
	}

	struct ReminderDesc;

	typedef list<ReminderDesc> ReminderList;

	typedef map<int64_t, ReminderList::iterator> UserMap;

	struct ReminderDesc {
		date::SteadyTimePoint when;
		UserMap::iterator user;
		ReminderDesc() : when(chrono::steady_clock::now()) {}
	};

	UserMap m_users;
	ReminderList m_reminder;

	mutex m_data_mutex;
	atomic_bool m_stopped;

	class Sleeper {
	public:
		Sleeper()
		: m_timeout_locked(false)
		, m_stopped(false)
		, m_interrupted(false)
		, m_has_timeout(false) {
			m_sleeper_thread = thread(&Reminder::Sleeper::SleeperThread, this);
		}

		~Sleeper() {
			if (!m_stopped)
				Stop();
		}

		void SleepUntil(const date::SteadyTimePoint &timepoint) {
			{
				unique_lock<mutex> wait_lock(m_sleeper_mutex);
				m_sleeper_timepoint = timepoint;
				LockTimeout();
			}

			m_sleeper.notify_one();

			unique_lock<mutex> wait_lock(m_sleeper_mutex);
			while(!m_stopped && !m_interrupted && m_has_timeout) {
				m_waiter.wait(wait_lock);
			}

			m_interrupted = false;

			UnlockTimeout();
		}

		void WakeUp() {
			//зовется из другого потока - лочки тут трогать нельзя
			m_interrupted = true;
			m_waiter.notify_one();
		}

		void Stop() {
			m_stopped = true;
			UnlockTimeout();

			m_sleeper.notify_one();
			m_waiter.notify_one();
			m_sleeper_thread.join();
		}
	private:
		bool m_timeout_locked;
		atomic_bool m_stopped;
		atomic_bool m_interrupted;
		atomic_bool m_has_timeout;

		thread m_sleeper_thread;

		timed_mutex m_timeout_mutex;
		mutex m_sleeper_mutex;

		condition_variable m_waiter;
		condition_variable m_sleeper;

		date::SteadyTimePoint m_sleeper_timepoint;

		void LockTimeout() {
			if (m_timeout_locked)
				return;

			m_timeout_mutex.lock();
			m_timeout_locked = true;
			m_has_timeout = true;
		}

		void UnlockTimeout() {
			if (!m_timeout_locked)
				return;

			m_has_timeout = false;
			m_timeout_locked = false;
			m_timeout_mutex.unlock();
		}

		void SleeperThread() {
			while(true) {
				//Нас могут разбудить раньше - лочку надо отпустить
				{
					unique_lock<mutex> wait_lock(m_sleeper_mutex);
					while(!m_stopped && !m_has_timeout) {
						m_sleeper.wait(wait_lock);
					}
				}

				if (m_stopped)
					return;

				if (m_timeout_mutex.try_lock_until(m_sleeper_timepoint)) {
					//если этот поток получил лочку, ему надо ее отпустить
					m_timeout_mutex.unlock();
				}

				m_has_timeout = false;
				m_waiter.notify_one();
			}
		}
	} m_sleeper;
} reminder;

/*
 * Класс, занимающийся приемом сообщений - входящий канал связи
 * Обрабатывает полученные сообщения, валидирует данные и
 * вызывает необходимые методы объектов хранения данных
 */
class IncomingListener {
public:
	void Start() {
		m_connection = Channel::Create(RABBITMQ_HOST);
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
		auto env = m_connection->BasicConsumeMessage(m_consumer);

		string msg = env->Message()->Body();
		ProcessMessage(msg);

		m_connection->BasicAck(env);
	}

	void ProcessMessage(string &msg) const {
		string msg_type = str::GetWord(msg, '\n');
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
			Debug("Failed to process request: " + string(e.what()));
		}
	}
};

int main() {
	try {
		thread reminder_thread(&Reminder::Process, &reminder);
		thread sender_thread(&Producer::SendMessages, &producer);

		IncomingListener().Start();

		reminder.Stop();
		reminder_thread.join();

		producer.Stop();
		sender_thread.join();
	} catch (const std::exception &e) {
		Debug("Unexpected error thrown: " + string(e.what()));
		return EXIT_FAILURE;
	} catch (...) {
		Debug("Unknown unexpected error thrown");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
