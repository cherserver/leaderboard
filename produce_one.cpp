#include <iostream>
#include <SimpleAmqpClient/SimpleAmqpClient.h>

#include <lb_defines.h>

using namespace std;
using namespace AmqpClient;

void PrintUsage() {
	cout << "Usage:" << endl;
	cout << "produce_one [MSG_TYPE] [PARAMS]" << endl;
	cout << "[MSG_TYPE] with [PARAMS] could be:" << endl;
	cout << "\tuser_registered [id] [name]" << endl;
	cout << "\tuser_renamed [id] [name]" << endl;
	cout << "\tuser_deal_won [id] [time] [amount]" << endl;
	cout << "\tuser_connected [id]" << endl;
	cout << "\tuser_disconnected [id]" << endl;
}

bool GetMessageContent(int argc, char *argv[], string &content) {
	if (argc < 3)
		return false;

	//Тут не заморачиваюсь с валидацией специально для проверки входных параметров
	string msg_type = argv[1];
	if (msg_type == MSG_USER_REGISTER || msg_type == MSG_USER_RENAME) {
		if (argc != 4)
			return false;
		content += msg_type;
		content += "\n";
		content += argv[2]; //id
		content += "\n";
		content += argv[3];	//name
	} else if (msg_type == MSG_USER_WON) {
		if (argc != 5)
			return false;
		content += msg_type;
		content += "\n";
		content += argv[2]; //id
		content += "\n";
		content += argv[3];	//time
		content += "\n";
		content += argv[4];	//amount
	} else if (msg_type == MSG_USER_CONNECT || msg_type == MSG_USER_DISCONNECT) {
		if (argc != 3)
			return false;
		content += msg_type;
		content += "\n";
		content += argv[2]; //id
	} else
		return false;

	return true;
}

int main(int argc, char *argv[]) {
	try {
		string content;
		if (!GetMessageContent(argc, argv, content)) {
			PrintUsage();
			return EXIT_FAILURE;
		}

		Channel::ptr_t connection(Channel::Create("localhost"));

		connection->DeclareQueue(LB_INPUT_QUEUE, false, false, false, false);
		connection->BasicPublish("", LB_INPUT_ROUTE, BasicMessage::Create(content));
	} catch (const std::exception &e) {
		cout << "Unexpected error thrown: " << e.what() << endl;
		return EXIT_FAILURE;
	} catch (...) {
		cout << "Unknown unexpected error thrown" << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
