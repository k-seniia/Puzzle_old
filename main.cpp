#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <regex>
#include <mutex>
#include <atomic>
#include <condition_variable>

using namespace std;

// Graph class to store and manage the adjacency list representation of the graph
class Graph
{
private:

	unordered_map<string, vector<string>> adj_list;	// Adjacency list mapping nodes to neighbors

public:

	// Adds a directed edge from one node to another
	void add_edge(const string& from, const string& to) { adj_list[from].push_back(to); }

	// Method to build the graph from a list of numbers
	void build_graph(const vector<string>& numbers)
	{
		for (const auto& from : numbers)
		{
			string key = from.substr(from.size() - 2);
			for (const auto& to : numbers)
				if (from != to && key == to.substr(0, 2))
					add_edge(from, to);
		}
	}

	// Retrieves neighbors of a given vertex
	const vector<string>& get_neighbors(const string& vertex) const
	{
		auto it = adj_list.find(vertex);
		if (it != adj_list.end())
			return it->second;
		static const vector<string> empty;
		return empty;
	}

	// Returns a list of all vertices in the graph
	vector<string> get_vertices() const
	{
		vector<string> vertices;
		for (const auto& pair : adj_list)
			vertices.push_back(pair.first);
		return vertices;
	}
};

// Class to find the longest paths in a directed graph using DFS (Depth-first search)
class LongestPath
{
private:

	vector<string> current_path;			// Tracks the current path during DFS
	vector<vector<string>> longest_paths;	// Stores all longest paths found
	unordered_map<string, bool> visited;	// Tracks visited nodes during DFS
	int max_length = 0;						// Length of the longest path found

	// Depth-first search to explore all paths from a given node
	void dfs(const string& current, const Graph& graph)
	{
		visited[current] = true;
		current_path.push_back(current);

		// Update longest path if a new maximum is found
		if (current_path.size() > max_length)
		{
			max_length = current_path.size();
			longest_paths.clear();
			longest_paths.push_back(current_path);
		}
		// Add path if it matches the current maximum length
		else if (current_path.size() == max_length)
			longest_paths.push_back(current_path);

		// Recursively visit all neighbors
		for (const auto& neighbor : graph.get_neighbors(current))
			if (!visited[neighbor])
				dfs(neighbor, graph);

		visited[current] = false;
		current_path.pop_back();
	}

public:

	// Method to find all longest paths in the graph
	void find_longest_paths(const Graph& graph)
	{
		atomic<bool> is_searching(true);
		mutex mtx;
		condition_variable cv;

		cout << "Starting the search for the longest sequence(s)...\n";
		thread progress_thread([&]() {
			unique_lock<mutex> lock(mtx);
			while (is_searching)
				if (cv.wait_for(lock, chrono::seconds(15)) == cv_status::timeout)
					cout << "The program is still searching for the longest sequence...\n";
			});

		auto vertices = graph.get_vertices();
		for (const auto& vertex : vertices)
		{
			visited.clear();
			current_path.clear();
			dfs(vertex, graph);
		}

		{
			lock_guard<mutex> lock(mtx);
			is_searching = false;
		}
		cv.notify_one();

		if (progress_thread.joinable())
			progress_thread.join();

		cout << "Search for the longest sequence(s) completed successfully.\n";
	}

	// Accessor for all longest paths
	vector<vector<string>> get_all_longest_paths() const { return longest_paths; }

	// Accessor for the maximum path length
	int get_max_length() const { return max_length; }
};

// Prints a path in the specified format
void print_path(const vector<string>& path)
{
	if (path.empty()) return;

	cout << path[0].substr(0, 4);
	for (size_t i = 1; i < path.size(); ++i)
		cout << path[i].substr(0, 2) << path[i].substr(2, 2);
	cout << path.back().substr(4, 2) << endl;
}

// Validates and retrieves user input for a yes/no choice
char get_validated_choice(const string& prompt)
{
	char choice;
	while (true)
	{
		cout << prompt; cin >> choice;
		choice = toupper(choice);
		if (choice == 'Y' || choice == 'N' || choice == 'y' || choice == 'n') break;
		else cout << "Invalid input. Please enter 'y' or 'n': ";
	}
	return choice;
}

int main()
{
	string file_path, temp;
	vector<string> numbers;
	ifstream fin;
	char retry;
	int attempts = 5;	// Counter for invalid file attempts

	// Inform the user about the algorithm and expected time
	cout << "This program uses a graph-based approach to find the longest sequences.\n";
	cout << "This program may take some time to execute for large datasets.\n";
	cout << "Please enter the path or name (if in the folder with this .exe) of the input file (for example source.txt): ";
	cin >> file_path;

	// Loop to handle file input with a limit on invalid attempts
	while (attempts--)
	{
		fin.open(file_path);
		if (fin.is_open())
		{
			cout << "File successfully opened.\n";
			break;
		} else if (attempts == 0)
		{
			cout << "Maximum number of attempts reached. Exiting program.\n";
			system("pause");
			return 1;
		} else
		{
			cerr << "Error: File not found.\n";
			retry = get_validated_choice("Would you like to try entering the file path again? (y/n): ");
			if (retry == 'N')
			{
				cout << "Exiting program. No file to process.\n";
				system("pause");
				return 1;
			}
			cout << "Please enter the path or name of the input file: ";
			cin >> file_path;
		}
	}

	// Read and validate each line in the file
	const regex valid_num("^\\d{6}$");
	while (fin >> temp)
		if (regex_match(temp, valid_num))
			numbers.push_back(temp);
	fin.close();
	if (numbers.empty())
	{
		cout << "Error: The file is empty or contains no valid data.\n";
		system("pause");
		return 1;
	}
	cout << "File successfully read. Loaded " << numbers.size() << " elements.\n";

	// Measure the time taken to process the data
	auto start = chrono::high_resolution_clock::now();

	// Build a graph from the list of numbers
	Graph graph;
	graph.build_graph(numbers);
	cout << "Graph construction completed. Total elements: " << graph.get_vertices().size() << endl;

	// Find the longest paths using depth-first search
	LongestPath lp_finder;
	lp_finder.find_longest_paths(graph);

	auto end = chrono::high_resolution_clock::now();
	chrono::duration<double> elapsed = end - start;

	cout << "Execution time: " << elapsed.count() / 60 << " minutes" << endl;
	cout << "Length of the longest sequence: " << lp_finder.get_max_length() << endl;
	cout << "Number of longest sequences: " << lp_finder.get_all_longest_paths().size() << endl;
	if (lp_finder.get_all_longest_paths().size() > 10)
	{
		cout << "There are " << lp_finder.get_all_longest_paths().size() << " sequences. ";
		char choice = get_validated_choice("Would you like to see them? (y/n): ");
		if (choice == 'N')
		{
			cout << "Exiting without displaying the sequences.\n";
			system("pause");
			return 0;
		}
	}

	cout << "Longest sequence(s):" << endl;
	int index = 1;
	for (const auto& path : lp_finder.get_all_longest_paths())
	{
		cout << index++ << ". " << endl;
		print_path(path);
		cout << endl;
	}
	system("pause");
	return 0;
}