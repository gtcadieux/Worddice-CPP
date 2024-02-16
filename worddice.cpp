/*
 *  Gabriel Cadieux
 *
 *  worddice.cpp
 *
 *  This program reads in a file of dice and a file of words and determines
 *  if the words can be spelled using the dice.
 *
 *  Input: Dice file and words file
 *  Output: The words that can be spelled using the dice
 */

#include <algorithm>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

typedef enum { SOURCE, SINK, WORD, DICE } Node_Type;

class Edge;

class Node {
public:
  Node(int id, Node_Type type, string word = ""); // constructor for nodes
  ~Node();                                        // default destructor
  bool has_letter(char c);
  friend ostream &operator<<(ostream &os, const Node &node);
  int id;               // node id
  Node_Type type;       // type of node it is (source, sink, word or dice)
  vector<bool> letters; // length 26 with letters contained in word set to 1
  bool visited;         // for BFS
  vector<Edge *> adj;   // adjacency list
  Edge *backedge;       // previous edge for Edmonds-Karp
};

class Edge {
public:
  // from -> to
  class Node *to;   // node edge is pointing to
  class Node *from; // node edge is pointing from
  Edge(class Node *to, class Node *from,
       bool reverse_edge = false); // constructor for edges
  ~Edge();                         // default destructor
  Edge *reverse;                   // edge going the other way
  int original;                    // original weight per edge
  int residual; // allows for updated weighting during Edmonds-Karp
};

class Graph {
public:
  Graph();      // constructor initializes graph with source node
  ~Graph();     // destructor to deallocate memory of graph
  Node *source; // not necessary but makes code more readable
  Node *sink;
  vector<Node *> nodes;    // holds the nodes
  vector<int> spellingIds; // order of flow to spell word
  int min_nodes;           // min number of dice nodes
  string word;
  vector<string> dice;                         // holds the dice
  void add_dice_to_graph(string die, int &id); // add dice nodes to graph
  void add_word_to_graph(string word,
                         int &id); // add word (letter) nodes to graph
  void add_die_edges(string word, int &id);
  bool BFS();        // breadth first search for Edmonds-Karp
  bool spell_word(); // runs Edmonds-Karp to see if we can spell the word
  void
  delete_word_from_graph(); // deletes the word nodes but leaves the dice nodes
  void print_node_order(string word); // print spelling Ids and word
};

void printGraph(Graph *graph);

int main(int argc, char *argv[]) {

  // Check for correct number of arguments
  if (argc != 3) {
    cout << "Usage: " << argv[0] << " <dice file> <word file>" << endl;
    return 0;
  }

  // Open dice file
  ifstream dice_file(argv[1]);
  ifstream word_file(argv[2]);

  // Check if files opened correctly
  if (!dice_file.is_open()) {
    cout << "Error: Could not open dice file" << endl;
    return 0;
  }
  if (!word_file.is_open()) {
    cout << "Error: Could not open word file" << endl;
    return 0;
  }

  // Create graph
  Graph *graph = new Graph();

  // Add dice to graph
  string die;
  int id = 1;

  while (getline(dice_file, die)) {
    graph->add_dice_to_graph(die, id);
  }

  string word;
  while (getline(word_file, word)) {
    id = graph->min_nodes;
    graph->word = word;
    graph->add_word_to_graph(word, id);
    graph->add_die_edges(word, id);
     //printGraph(graph);
    if (graph->spell_word()) {
		for (int i = graph->min_nodes; i < (graph->nodes.size() - 1); i++) {
			for (int j = 0; j < graph->nodes[i]->adj.size(); j++) {
				if (graph->nodes[i]->adj[j]->original == 1) {
					if ( i == (graph->nodes.size() - 2)) {
						cout << (graph->nodes[i]->adj[j]->to->id - 1);
					}
					else {
					cout <<	(graph->nodes[i]->adj[j]->to->id - 1) << ",";
					}
				}
			}
		}
		cout << ": " << word << "\n";
	}
	else {
		cout << "Cannot spell " << word << "\n";
	}
    graph->delete_word_from_graph();
  }

  // Close files
  dice_file.close();
  word_file.close();

  // printGraph(graph);

  // Delete graph
  delete graph;

  return 0;
}

// Function Definitions

// Create constructors for each class
Node::Node(int id, Node_Type type, string word) {
  this->id = id;
  this->type = type;
  this->letters = vector<bool>(26, 0);
  this->adj = vector<Edge *>();
  this->backedge = NULL;
  if (type == WORD || type == DICE) {
    for (int i = 0; i < word.length(); i++) {
      this->letters[word[i] - 'A'] = 1;
    }
  }
}

Edge::Edge(Node *to, Node *from, bool reverse_edge) {
  this->to = to;
  this->from = from;
  if (!reverse_edge) {
	this->original = 1;
	this->residual = 0;
    this->reverse = new Edge(from, to, true);
    this->reverse->reverse = this;
  } 
  else {
	this->original = 0;
	this->residual = 1;
    //this->reverse = NULL;
  }
}

Graph::Graph() {
  this->source = new Node(0, SOURCE);
  this->nodes = vector<Node *>();
  this->spellingIds = vector<int>();
  this->min_nodes = 1;
  this->word = "";
  this->nodes.push_back(this->source);
}

// Create destructors for each class

Node::~Node() {
	//cerr << "Node popped\n";
}

Edge::~Edge() {
	//cerr << "Edge popped\n";
  if (this->reverse != NULL) {
    delete this->reverse;
  }
}

Graph::~Graph() {
  for (int i = 0; i < this->min_nodes; i++) {
    delete this->nodes[i];
  }
}

// Implement dice input
// implement dump_node or operator<< to verify dice input
void Graph::add_dice_to_graph(string die, int &id) {
  Node *node = new Node(id, DICE, die);
  Edge *edge = new Edge(node, this->source);
  // node->adj.push_back(edge);
  this->source->adj.push_back(edge);
  this->nodes.push_back(node);
  dice.push_back(die);
  id++;
  min_nodes++;
}

// Implement word input

void Graph::add_word_to_graph(string word, int &id) {
  // this->word = word;
  sink = new Node(min_nodes + word.length(), SINK);
  for (int i = 0; i < word.length(); i++) {
    Node *node = new Node(id, WORD, word.substr(i, 1));
    Edge *edge = new Edge(this->sink, node);
    node->adj.push_back(edge);
    this->nodes.push_back(node);
    id++;
  }
  this->nodes.push_back(sink);
}

// write a simple function to print the graph
// this will be useful for debugging

void printGraph(Graph *graph) {
  for (int i = 0; i < graph->nodes.size(); i++) {
    cout << "Node " << graph->nodes[i]->id << ": ";
    if (graph->nodes[i]->type == SOURCE) {
      cout << "SOURCE";
    } else if (graph->nodes[i]->type == SINK) {
      cout << "SINK";
    } else if (graph->nodes[i]->type == DICE) {
      for (int j = 0; j < 26; j++) {
        if (graph->nodes[i]->letters[j])
          cout << (char)(j + 'A');
      }

    } else if (graph->nodes[i]->type == WORD) {
      for (int j = 0; j < graph->nodes[i]->letters.size(); j++) {
        if (graph->nodes[i]->letters[j]) {
          cout << (char)(j + 'A');
        }
      }
    }
    cout << " Edges to ";
    for (int j = 0; j < graph->nodes[i]->adj.size(); j++) {
      cout << graph->nodes[i]->adj[j]->to->id << " ";
    }
    cout << endl;
  }
}

void Graph::delete_word_from_graph() {
  for (int i = nodes.size(); i > min_nodes; i--) {
    this->nodes.pop_back();
  }

  for (int i = 1; i < min_nodes; i++) {
    this->nodes[i]->adj.clear();
  }
  for (int i = 0; i < min_nodes - 1; i++) {
	this->source->adj[i]->original = 1;
	this->source->adj[i]->residual = 0;
  }
  
}

void Graph::add_die_edges(string word, int &id) {
  for (int i = 0; i < this->nodes.size(); i++) {
    if (this->nodes[i]->type == DICE) {
      for (int j = 0; j < this->nodes.size(); j++) {
        if (this->nodes[j]->type == WORD) {
          for (int k = 0; k < 26; k++) {
            if ((this->nodes[i]->letters[k] == 1) &&
                (this->nodes[j]->letters[k] == 1)) {
              Edge *edge = new Edge(this->nodes[j], this->nodes[i]);
              this->nodes[i]->adj.push_back(edge);
			  this->nodes[j]->adj.push_back(edge->reverse);
            }
          }
        }
      }
    }
  }
}

bool Graph::BFS() {
  // Initialize BFS
  queue<Node *> q;
  for (auto n : nodes) {
    n->visited = false;
    n->backedge = nullptr;
  }

  // Add source to queue
  source->visited = true;
  q.push(source);

  // BFS
  while (!q.empty()) {
    Node *current = q.front();
    q.pop();

    // Check each neighbor
    for (auto e : current->adj) {
      Node *neighbor = e->to;
      // Make sure the neighbor has not been visited and has flow
      if ((neighbor->visited == false) && (e->original == 1)) {
        // Add the neighbor to the queue
        neighbor->visited = true;
        neighbor->backedge = e;
        q.push(neighbor);
		if (neighbor->type == SINK) {
			return true;
		}

      }
    }
  }

  return false;
}

bool Graph::spell_word() {
	int count = 0;
  while (BFS()) {
	  Node *current;
	  this->sink->backedge->original = 0;
	  this->sink->backedge->residual = 1;
	  current = this->sink->backedge->from;
	  while (current->type != SOURCE) {
		current->backedge->original = 0;
		current->backedge->residual = 1;
		if (current->backedge->reverse == NULL) {
		
		}
		else {
			current->backedge->reverse->original = 1;
			current->backedge->reverse->residual = 0;
		}

		current = current->backedge->from;
	}
  }
  
  for (int i = 0; i < this->nodes.size(); i++) {
	if (this->nodes[i]->type == WORD) {
		if (this->nodes[i]->adj[0]->residual == 1) {
			count++;
		}
	}
  }
  if (count == word.length()) {
	  count = 0;
	return true;
  }
  else {
	  count = 0;
	return false;
  }
}
