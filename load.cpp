#include <chrono>
#include <iostream>
#include <SimpleAmqpClient/SimpleAmqpClient.h>

#include <lb_defines.h>
#include <lb_functions.h>

using namespace std;
using namespace AmqpClient;

int main() {
	try {
		int64_t load = 1000;

		Channel::ptr_t connection(Channel::Create(RABBITMQ_HOST));
		connection->DeclareQueue(LB_INPUT_QUEUE, false, false, false, false);

		for (int64_t cnt = 1; cnt <= load; ++cnt) {
			const string id = str::Str(cnt);
			connection->BasicPublish("", LB_INPUT_ROUTE,
				BasicMessage::Create(
						MSG_USER_REGISTER +
						"\n" + id +
						"\nuser" + id
				)
			);
		}

		for (int64_t cnt = 1; cnt <= load; ++cnt) {
			const string id = str::Str(cnt);
			connection->BasicPublish("", LB_INPUT_ROUTE,
				BasicMessage::Create(
						MSG_USER_WON +
						"\n" + id +
						"\n" + date::Format(chrono::system_clock::now()) +
						"\n" + id

				)
			);
		}
	} catch (const std::exception &e) {
		cout << "Unexpected error thrown: " << e.what() << endl;
		return EXIT_FAILURE;
	} catch (...) {
		cout << "Unknown unexpected error thrown" << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
