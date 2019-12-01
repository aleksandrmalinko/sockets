#include <iostream>

char set_random_characters();

using namespace std;

int main() {
    for (int i = 'a'; i < 'z'; i++) {
        set_random_characters();
    }
    return 0;
}

char set_random_characters() {
    char c = char('a' + rand() % ('z' - 'a'));
    cout << c << " ";
    return c;
}
