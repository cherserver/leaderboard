#include <chrono>
#include <iostream>
#include <SimpleAmqpClient/SimpleAmqpClient.h>

#include <lb_defines.h>
#include <lb_functions.h>

using namespace std;
using namespace AmqpClient;

int main() {
	try {
		Channel::ptr_t connection(Channel::Create("localhost"));
		connection->DeclareQueue(LB_OUTPUT_QUEUE, false, false, false, false);
		string consumer = connection->BasicConsume(LB_OUTPUT_QUEUE, "", true, true);
		while(true) {
			auto env = connection->BasicConsumeMessage(consumer);
			cout << date::Format(chrono::system_clock::now()) << "--------------" << endl
					<< env->Message()->Body() << endl;
		}
	} catch (const std::exception &e) {
		cout << "Unexpected error thrown: " << e.what() << endl;
		return EXIT_FAILURE;
	} catch (...) {
		cout << "Unknown unexpected error thrown" << endl;
		return EXIT_FAILURE;
	}
}
