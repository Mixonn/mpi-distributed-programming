#include <string>
#include <iostream>
#include "queue.hpp"

using namespace std;


int main(int argc, char** argv) {
    PriorityQueue queue;
    cout << queue.to_string();

    queue.put(Node(1, 5, 2));
    cout << queue.to_string() << endl;

    queue.put(Node(1, 9, 3));
    cout << queue.to_string() << endl;

    queue.put(Node(1, 7, 1));
    cout << queue.to_string() << endl;

    queue.put(Node(0, 11, 4));
    cout << queue.to_string() << endl;

    queue.put(Node(1, 3, 5));
    cout << queue.to_string() << endl;

    cout << "POPPING" << endl;
    queue.pop(1);
    cout << queue.to_string() << endl;

    queue.pop(3);
    cout << queue.to_string() << endl;

    queue.pop(5);
    cout << queue.to_string() << endl;

    queue.pop(2);
    cout << queue.to_string() << endl;

    queue.pop(4);
    cout << queue.to_string() << endl;
}