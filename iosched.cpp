#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <stack>
#include <map>
#include <list>

using namespace std;

//-------------------- STEP 1 : Create the IO operation structure --------------------
struct IO_op {
    int oid; // id of the operation. Could also use the arrival_time since there are no overlap
    int arrival_time; // time when IO operation is issued
    int track; // track that is accessed

    bool isCompleted; // check if the IO_operation is completed
    int start_time; // time when the IO operation starts
    int end_time; // time when the IO operation ends
    int turnaround_time; // turn around time. Used to compute summary
    int wait_time; // wait time from being submitted to start being executed

    IO_op(int oid_, int arrival_time_, int track_) {
        oid = oid_;
        arrival_time = arrival_time_;
        track = track_;

        isCompleted = false;
        start_time = -1;
        end_time = -1;
        turnaround_time = -1;
    }
};


//-------------------- STEP 2 : Read Input File and initialize the IO operations queue --------------------
// Now, we can read the input file and initialize the IO operations queue

// We choose vector over queue because we wanna keep the input for the summary
vector<IO_op*> IO_ops_input_queue; // vector of all the IO operations in order of their appearance. 
int hand_input = 0; // index of current input IO operation. Used to imitate the behavior of a queue
int size_IO_ops_input_queue;

void readInput(istream& input_file) {
    
    string line;
    // We skip the first comments lines
    while (getline(input_file, line)) {
        if (line[0] != '#') {
            break;
        }
    }

    double arrival_time, track;

    int count= 0; // Same as oid. We use the order of arrival as the oid of the IO operation
    // We process the first io_operation manually because it is not taken into account in the future while loop
    istringstream issVMA(line);
    issVMA >> arrival_time >> track;    
    IO_ops_input_queue.push_back( new IO_op(count, arrival_time, track) );
    count++;
    while ( input_file >> arrival_time >> track ) {
        IO_op* io_op = new IO_op(count, arrival_time, track);
        IO_ops_input_queue.push_back(io_op);
        count++;
    }
    size_IO_ops_input_queue = count;

};


//-------------------- STEP 3 : Create Abstract class for Scheduler Algorithms --------------------

class Scheduler {
    public:
        int head;
        IO_op* curr_io_op;

        virtual IO_op* strategy() = 0; // Choose next IO operation given the request queue. To be implemented by each scheduler
        virtual void move_head() = 0; // Move head. To be implemented by each scheduler
        virtual void add_request() = 0; // Add next input IO operation to request queue. Scheduler dependant because it depends on if the request queue is a queue, a vector etc
        virtual bool hasRequest() = 0; // Check if the request queue is empty or not. Scheduler dependant because it depends on if the request queue is a queue or a vector etc

        Scheduler() {
            head = 0;
            curr_io_op = NULL;
        }

};

//-------------------- STEP 4 : Create the different Scheduler Algorithms --------------------

class FIFO: public Scheduler {
    queue<IO_op*> request_queue;

    IO_op* strategy() {
        if (request_queue.empty()) {
                return NULL;
        } else {
            IO_op* next_io_op = request_queue.front();
            request_queue.pop();
            curr_io_op = next_io_op;
            return next_io_op;
        }
    }

    // Move head toward a target track
    void move_head() {
        if ( head < curr_io_op->track ) {
            head++;
        } else {
            head--;
        }
        if (head == curr_io_op->track) {
            curr_io_op->isCompleted = true;
        }
    };

    void add_request() {
        IO_op* next_io_op_input = IO_ops_input_queue[hand_input];
        hand_input++;
        request_queue.push(next_io_op_input);
    }

    bool hasRequest() {
        return !(request_queue.empty());
    }


};


class SSTF: public Scheduler {
    vector<IO_op*> request_queue;

    IO_op* strategy() {
        if (request_queue.empty()) {
                return NULL;
        } else {

            //IO_op* next_io_op = request_queue.front();
            IO_op* next_io_op = NULL;
            //int shortest_distance = abs(next_io_op->track - head);
            int shortest_distance = -1;
            //vector<IO_op*>::iterator shortest_it_op = request_queue.begin(); // To erase from the queue later
            vector<IO_op*>::iterator shortest_it_op; // To erase from the queue later

            for (vector<IO_op*>::iterator it_op = request_queue.begin(); it_op != request_queue.end(); it_op++) {
                IO_op* io_op = *it_op;
                int distance = abs(io_op->track - head);
                if ( (next_io_op == NULL) || (distance < shortest_distance) ) {
                    next_io_op = io_op;
                    shortest_distance = distance;
                    shortest_it_op = it_op;
                }
            }
            // At the end of the search, we found the shortest seek time request.
            // We remove it from the request queue and return it
            request_queue.erase(shortest_it_op);
            curr_io_op = next_io_op;
            return next_io_op;

        }
    }

    // Move head toward a target track
    void move_head() {
        // Careful of edge case : if head is already on the track of a new operation, we don't move it
        if ( head < curr_io_op->track ) {
            head++;
        } else if ( head > curr_io_op->track ) {
            head--;
        }

        if (head == curr_io_op->track) {
            curr_io_op->isCompleted = true;
        }
    };

    void add_request() {
        IO_op* next_io_op_input = IO_ops_input_queue[hand_input];
        hand_input++;
        request_queue.push_back(next_io_op_input);
    }

    bool hasRequest() {
        return !(request_queue.empty());
    }


};


class LOOK: public Scheduler {
    vector<IO_op*> request_queue;
    bool going_forward; // This bool decides if we're going forward or backward (direction of the look)

    public :
        LOOK():Scheduler() {
            going_forward = true; // We assume that we start with the head at 0 so we move forward
        }

    // We must pick the closest request in our direction
    // It's just like SSTF except we filter out the request which are not in our direction
    IO_op* strategy() {
        if (request_queue.empty()) {
                return NULL;
        } else {

            IO_op* next_io_op = NULL;
            int shortest_distance = -1;
            vector<IO_op*>::iterator shortest_it_op; // To erase from the queue later

            for (vector<IO_op*>::iterator it_op = request_queue.begin(); it_op != request_queue.end(); it_op++) {
                IO_op* io_op = *it_op;
                int distance = io_op->track - head;
                // the conditions check if we are going in the scheduler's direction
                if ( ( (distance >= 0) && (going_forward) ) 
                    || ( (distance <= 0) && !going_forward ) ) {

                    if ( (next_io_op == NULL) || (abs(distance) < abs(shortest_distance)) ) {
                        next_io_op = io_op;
                        shortest_distance = distance;
                        shortest_it_op = it_op;
                    }

                }
            }
            // We either found a request or we didn't. For the latter, we have to reverse the direction and do the same
            if (next_io_op == NULL) {
                going_forward = !going_forward;
                for (vector<IO_op*>::iterator it_op = request_queue.begin(); it_op != request_queue.end(); it_op++) {
                    IO_op* io_op = *it_op;
                    int distance = io_op->track - head;
                    // the conditions check if we are going in the scheduler's direction
                    if ( ( (distance >= 0) && (going_forward) ) 
                        || ( (distance <= 0) && !going_forward ) ) {

                        if ( (next_io_op == NULL) || (abs(distance) < abs(shortest_distance)) ) {
                            next_io_op = io_op;
                            shortest_distance = distance;
                            shortest_it_op = it_op;
                        }

                    }
                }
            } 

            // At the end of the search, we found the shortest seek time request in the scheduler request.
            // We remove it from the request queue and return it
            request_queue.erase(shortest_it_op);
            curr_io_op = next_io_op;
            return next_io_op;

        }
    }

    // Move head toward a target track
    void move_head() {
        // Careful of edge case : if head is already on the track of a new operation, we don't move it
        if ( head < curr_io_op->track ) {
            head++;
        } else if ( head > curr_io_op->track ) {
            head--;
        }

        if (head == curr_io_op->track) {
            curr_io_op->isCompleted = true;
        }
    };

    void add_request() {
        IO_op* next_io_op_input = IO_ops_input_queue[hand_input];
        hand_input++;
        request_queue.push_back(next_io_op_input);
    }

    bool hasRequest() {
        return !(request_queue.empty());
    }


};


class CLOOK: public Scheduler {
    // Same as LOOK, we just make minor changes in the strategy
    vector<IO_op*> request_queue;


    // We must pick the closest request in our direction
    // It's just like SSTF except we filter out the request which are not in our direction
    IO_op* strategy() {
        if (request_queue.empty()) {
                return NULL;
        } else {

            IO_op* next_io_op = NULL;
            int shortest_distance = -1;
            vector<IO_op*>::iterator shortest_it_op; // To erase from the queue later

            for (vector<IO_op*>::iterator it_op = request_queue.begin(); it_op != request_queue.end(); it_op++) {
                IO_op* io_op = *it_op;
                int distance = io_op->track - head;
                // the conditions check if we are going in the scheduler's direction which is  always forward
                if ( distance >= 0 ) {

                    if ( (next_io_op == NULL) || (abs(distance) < abs(shortest_distance)) ) {
                        next_io_op = io_op;
                        shortest_distance = distance;
                        shortest_it_op = it_op;
                    }

                }
            }
            // We either found a request or we didn't. For the latter, we have to circle back to track 0 and look again
            // To do so, we copy the previous loop and we change the head to 0
            // We can't type head = 0 because it is logically false, the scheduler's head doesn't teleport to 0 like that
            if (next_io_op == NULL) {
                for (vector<IO_op*>::iterator it_op = request_queue.begin(); it_op != request_queue.end(); it_op++) {
                    IO_op* io_op = *it_op;
                    int distance = io_op->track - 0; // <== Here we change
                    // the conditions check if we are going in the scheduler's direction
                    // It's useless here since we look from track 0, but who cares
                    if ( distance >= 0 ) {

                        if ( (next_io_op == NULL) || (abs(distance) < abs(shortest_distance)) ) {
                            next_io_op = io_op;
                            shortest_distance = distance;
                            shortest_it_op = it_op;
                        }

                    }
                }

            } 

            // At the end of the search, we found the shortest seek time request in the scheduler request.
            // We remove it from the request queue and return it
            request_queue.erase(shortest_it_op);
            curr_io_op = next_io_op;
            return next_io_op;

        }
    }

    // Move head toward a target track
    void move_head() {
        // Careful of edge case : if head is already on the track of a new operation, we don't move it
        if ( head < curr_io_op->track ) {
            head++;
        } else if ( head > curr_io_op->track ) {
            head--;
        }

        if (head == curr_io_op->track) {
            curr_io_op->isCompleted = true;
        }
    };

    void add_request() {
        IO_op* next_io_op_input = IO_ops_input_queue[hand_input];
        hand_input++;
        request_queue.push_back(next_io_op_input);
    }

    bool hasRequest() {
        return !(request_queue.empty());
    }


};


class FLOOK: public Scheduler {
    // Create POINTERS to add_queue and active_queue
    vector<IO_op*>* add_queue;
    vector<IO_op*>* active_queue;
    bool going_forward; // This bool decides if we're going forward or backward (direction of the look)

    public :
        FLOOK():Scheduler() {
            going_forward = true; // We assume that we start with the head at 0 so we move forward
            add_queue = new vector<IO_op*>();
            active_queue = new vector<IO_op*>();
        }
    // It's just like LOOK, just need to swap the queue when the active_queue is empty..
    IO_op* strategy() {
        if (add_queue->empty() && active_queue->empty()) {
                return NULL;
        } else {
            
            // First we check if active queue is empty or not. If empty, we swap
            if (active_queue->empty()) {
                swap(active_queue, add_queue);
            }

            // Now we know for sure that the active queue is NOT empty
            // If it is still empty, it means that the add_queue was also empty
            // But if that was the case, we would have returned NULL previously

            // Now we do just like LOOK but with the active_queue. It's the same code literally
            IO_op* next_io_op = NULL;
            int shortest_distance = -1;
            vector<IO_op*>::iterator shortest_it_op; // To erase from the queue later

            for (vector<IO_op*>::iterator it_op = active_queue->begin(); it_op != active_queue->end(); it_op++) {
                IO_op* io_op = *it_op;
                int distance = io_op->track - head;
                // the conditions check if we are going in the scheduler's direction
                if ( ( (distance >= 0) && (going_forward) ) 
                    || ( (distance <= 0) && !going_forward ) ) {

                    if ( (next_io_op == NULL) || (abs(distance) < abs(shortest_distance)) ) {
                        next_io_op = io_op;
                        shortest_distance = distance;
                        shortest_it_op = it_op;
                    }

                }
            }
            // We either found a request or we didn't. For the latter, we have to reverse the direction and do the same as before
            if (next_io_op == NULL) {
                going_forward = !going_forward; // Here we make a change
                for (vector<IO_op*>::iterator it_op = active_queue->begin(); it_op != active_queue->end(); it_op++) {
                    IO_op* io_op = *it_op;
                    int distance = io_op->track - head;
                    // the conditions check if we are going in the scheduler's direction
                    if ( ( (distance >= 0) && (going_forward) ) 
                        || ( (distance <= 0) && !going_forward ) ) {

                        if ( (next_io_op == NULL) || (abs(distance) < abs(shortest_distance)) ) {
                            next_io_op = io_op;
                            shortest_distance = distance;
                            shortest_it_op = it_op;
                        }

                    }
                }
            } 

            // At the end of the search, we found the shortest seek time request in the scheduler request.
            // We remove it from the request queue and return it
            active_queue->erase(shortest_it_op);
            curr_io_op = next_io_op;
            return next_io_op;

        }
    }

    // Move head toward a target track
    void move_head() {
        // Careful of edge case : if head is already on the track of a new operation, we don't move it
        if ( head < curr_io_op->track ) {
            head++;
        } else if ( head > curr_io_op->track ) {
            head--;
        }

        if (head == curr_io_op->track) {
            curr_io_op->isCompleted = true;
        }
    };

    void add_request() {
        IO_op* next_io_op_input = IO_ops_input_queue[hand_input];
        hand_input++;
        add_queue->push_back(next_io_op_input);
    }

    bool hasRequest() {
        // If the active queue is empty, we would swap so we need to check the add_queue aswell
        return !( active_queue->empty() && add_queue->empty() );
    }


};




//-------------------- STEP 5 : Create the simulator --------------------

struct Simulator {
    int CLOCK; // internal clock
    IO_op* curr_io_op; // Current IO operation

    int tot_movement; // total total number of tracks the head had to be moved
    double avg_turnaround; // average turnaround time per operation from time of submission to time of completion
    double avg_wait_time; // average wait time per operation (time from submission to issue of IO request to start disk operation)
    int max_wait_time; // maximum wait time for any IO operation.
    
    Scheduler* scheduler;

    Simulator(Scheduler* scheduler_) {
        CLOCK = -1;
        scheduler = scheduler_;
        curr_io_op = scheduler->curr_io_op;

        avg_turnaround = 0;
        avg_wait_time = 0;
        max_wait_time = 0;
        tot_movement = 0;
    }


    void compute_info(IO_op* io_op) {
        io_op->end_time = CLOCK;
        io_op->turnaround_time = CLOCK - io_op->arrival_time;
    }


    void simulation() {
        CLOCK = 1; // Initialize clock
        while (true) {
            if (CLOCK == 889) {
                int caca = 0;
            }
            curr_io_op = scheduler->curr_io_op;

            if (hand_input < size_IO_ops_input_queue && IO_ops_input_queue[hand_input]->arrival_time == CLOCK) {
                scheduler->add_request();
            }
            if ( curr_io_op != NULL && curr_io_op->isCompleted ) {
                compute_info(curr_io_op);
                scheduler->curr_io_op = NULL;
                curr_io_op = NULL;
            }
            if (curr_io_op == NULL) {
                if ( scheduler->hasRequest() ) {
                    curr_io_op = scheduler->strategy();
                    curr_io_op->start_time = CLOCK;
                    curr_io_op->wait_time = CLOCK - curr_io_op->arrival_time;
                } 
                else if ( !(scheduler->hasRequest()) && hand_input == size_IO_ops_input_queue ) {
                    return;
                }
            }
            if (curr_io_op != NULL) {
                int temp_past_head = scheduler->head;
                scheduler->move_head();
                // Check if head had to be moved
                if (temp_past_head != scheduler->head) {
                    tot_movement++;
                } 
                // Else, it means the head is already on the new IO operation's track so we choose another
                // Warning edge case : Because one hand movement = 1 time unit, this must happen within the SAME time unit
                // So we must skip the "CLOCK++" by using a continue statement
                else {
                    continue;
                }
            }

            CLOCK++;

        } // end of while loop
    } // end of simulation function

    void print_summary() {
        for (vector<IO_op*>::iterator op_it = IO_ops_input_queue.begin(); op_it != IO_ops_input_queue.end(); op_it++) {
            IO_op* io_op = *op_it;

            // Print stat
            printf("%5d: %5d %5d %5d\n", io_op->oid, io_op->arrival_time, io_op->start_time, io_op->end_time);

            // Compute summary statistic
            avg_turnaround += (double) io_op->turnaround_time;
            avg_wait_time += (double) io_op->wait_time;
            if (io_op->wait_time > max_wait_time) {
                max_wait_time = io_op->wait_time;
            }
        }

        avg_turnaround /= (double) size_IO_ops_input_queue;
        avg_wait_time /= (double) size_IO_ops_input_queue;


        printf("SUM: %d %d %.2lf %.2lf %d\n",
                CLOCK, tot_movement, avg_turnaround, avg_wait_time, max_wait_time);
    }

}; // End of struct simulator



int main(int argc, char *argv[]) {
    
    bool sflag = false;
    bool vflag = false;
    bool qflag = false;
    bool fflag = false;
    char *svalue = NULL;
    int o;

    
    opterr = 0;

    while ((o = getopt (argc, argv, "s:vqf")) != -1)
        switch (o)
        {
        case 's':
            sflag = true;
            if (optarg[0] == '-'){
                fprintf (stderr, "Option -s requires an argument.\n");
                return -1;
            }
            svalue = optarg;
            break;
        case 'v':
            vflag = true;
            break;
        case 'q':
            qflag = true;
            break;
        case 'f':
            fflag = true;
            break;
        case '?':
            if (optopt == 's') {
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            }
            else if (isprint (optopt)) {
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            }
            else {
            fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
            }
            return 1;
        default:
            abort ();
        }

    if (argc - optind < 1 ) { 
        printf("Please give an input file\n"); 
        return -1; 
    }
    else if (argc - optind > 1) { 
        printf("Please put only 1 input file\n"); 
        return -1; 
    }
    // Now we know we have an input file and a random file as non-option arguments
    ifstream input_file ( argv[optind] ); // input file

    // Check if file opening succeeded
    if ( !input_file.is_open() ) {
        cout<< "Could not open the input file \n"; 
        return -1;
    }

    // Process input file to initialize the IO operations
    readInput(input_file);


    // Define the scheduler
    Scheduler* scheduler;
    switch (svalue[0]) {

        case 'i' : {
            scheduler = new FIFO();
            break;
        }
        case 'j' : {
            scheduler = new SSTF();
            break;
        }
        case 's' : {
            scheduler = new LOOK();
            break;
        }
        case 'c' : {
            scheduler = new CLOOK();
            break;
        }
        case 'f' : {
            scheduler = new FLOOK();
            break;
        }

    }

    Simulator simulator = Simulator(scheduler);
    simulator.simulation();

    simulator.print_summary();


}