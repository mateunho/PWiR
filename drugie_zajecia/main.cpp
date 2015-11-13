#include <iostream>
#include <unistd.h>
#include <signal.h>

using namespace std;


bool isLooping = true;

/**
 * @brief sigterm_handler
 * Handles system signal `SIGTERM` sent i.e. from the console
 */
void sigterm_handler(int);


int main()
{
	// ustawiamy obsługę sygnału `TERM` przez funkcję `sigterm_handler`
	signal(SIGTERM, sigterm_handler);

	// cykliczne wyświetlanie `PIDu` bieżącego procesu
    while (isLooping) {
        cout << getpid() << endl;
        sleep(1);
    }

    /**
		w tym momencie ręcznie wysyłamy sygnał `TERM` za pomocą
        `$> kill -TERM PID`
        gdzie `PID` jest numerem bieżącego procesu
	*/

	// opóźnienie dla odczytania w konsoli efektu wysłania sygnału `TERM`
    sleep(5);

    // uruchomienie GStreamera z pewnymi argumentami
    execl("/usr/bin/gst-launch-0.10", "gst-launch",
          "videotestsrc", "!", "ximagesink", (char*)NULL);

	// to nie powinno pojawić się w konsoli
    cout << "Zakończenie ^_^" << endl;

    return 0;
}


void sigterm_handler(int arg)
{
    cout << arg << " SIGTERM handler" << endl;
    isLooping = false;
}
