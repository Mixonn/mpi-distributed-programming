#include <string>
#include <iostream>
#include "Queue.cpp"

int main(int argc, char** argv) {
    Priority_Queue queue;
    cout << queue.to_string();
    Node *node = new Node();
    node->tid = 5;
    node->clk = 2;
    queue.put(node);
    cout << queue.to_string() << endl;

    node = new Node();
    node->tid = 9;
    node->clk = 3;
    queue.put(node);
    cout << queue.to_string() << endl;

    node = new Node();
    node->tid = 7;
    node->clk = 1;
    queue.put(node);
    cout << queue.to_string() << endl;

    node = new Node();
    node->tid = 4;
    node->clk = 1;
    queue.put(node);
    cout << queue.to_string() << endl;

    Node *node2 = new Node();
    node2->tid = 2;
    node2->clk = 1;
    queue.put(node2);
    cout << queue.to_string() << endl;

    cout << "POPPING" << endl;
    queue.pop(4);
    cout << queue.to_string() << endl;

    queue.pop(5);
    cout << queue.to_string() << endl;

    queue.pop(9);
    cout << queue.to_string() << endl;

    queue.pop(7);
    cout << queue.to_string() << endl;

    queue.pop(2);
    cout << queue.to_string() << endl;
}