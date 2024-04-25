# Introduction

This project is implemented by Mickey Chang as the Final Project of Harvard University CS146*(24Spring)*. In this project, I implemented four different kinds of branch predictors, as shown below, and tested their performance across different SPEC2017 benchmarks.

# Source Code

Here lists the files I submit:

1. `twoLevelPredictor.cpp`
2. `gsharePredictor.cpp`
3. `tournamentPredictor.cpp`
4. `perceptronPredictor.cpp`



## Main Function Descriptions

### Two Level Adaptive Predictor

1.`HHRT_init()` ,`HHRT_update()` : Initialize and update the Hash History Register Table.

2.`PT_init()`,`PT_update()`: Initialize and update the Pattern Table.

3.`HHRT_hash_address()`: Hash the address into the table.

4.`HHRT_prediction()`: Make prediction.



### GSHARE Predictor

1.`GHR_init()` ,`GHR_update()` : Initialize and update the Global History Register.

2.`PT_init()`,`PT_update()`: Initialize and update the Pattern Table.

3.`XOR_address()`: XOR the address with the GHR.

4.`GHR_prediction()`: Make prediction.



### Tournament Predictor

1.`Chooser_init()`,`Chooser_update()`: Initialize and update the Chooser.

2.`Chooser_prediction()`: Make prediction about choosing which predictor.

3.`GHR_init()` ,`GHR_update()` : Initialize and update the Global History Register. *(For Global Predictor)*

4.`HHRT_init()` ,`HHRT_update()` : Initialize and update the Hash History Register Table. *(For Local Predictor)*

5.`Global_PT_init()`,`Global_PT_update()`:Initialize and update the Pattern Table. *(For Global Predictor)*

6.`Local_PT_init()`,`Local_PT_update()`:Initialize and update the Pattern Table. *(For Local Predictor)*

7.`GHR_prediction()`: Make prediction. *(For Global Predictor)*

8.`HHRT_prediction()`: Make prediction. *(For Local Predictor)*



### Perceptron Predictor

1.`GHR_init()` ,`GHR_update()` : Initialize and update the Global History Register.

2.`PERT_init()`: Initialize the Perceptron Table.

3.`PERT_train()`: Train the perceptron after prediction being done.

4.`PERT_prediction()`,`getSum()`: Make prediction.



# Benchmarks

Here lists the benchmarks I use to test these predictors and their descriptions:

*(You may find them in the **CPU2017** folder)*

1. **libquantum**

2. **mcf**

   From SPEC2017: 

   > The program is designed for the solution of single-depot vehicle scheduling (sub-)problems occurring in the planning process of public transportation companies. It considers one single depot and a homogeneous vehicle fleet. Based on a line plan and service frequencies, so-called timetabled trips with fixed departure/arrival locations and times are derived. Each of these timetabled trips has to be serviced by exactly one vehicle. The links between these trips are so-called dead-head trips. In addition, there are pull-out and pull-in trips for leaving and entering the depot.

3. **deepsjeng**

   From SPEC2017:

   > It attempts to find the best move via a combination of alpha-beta tree searching, advanced move ordering, ositional evaluation and heuristic forward pruning. Practically, it will explore the tree of variations resulting from a given position to a given base depth, extending interesting variations but discarding doubtful or irrelevant ones. From this tree the optimal line of play for both players ("principal variation") is determined, as well as a score reflecting the balance of power between the two.

4. **leela**

   From SPEC2017:

   > It combines recent advances in computer go in a single package. For each position, a restricted set of candidate moves is selected based on a ratings calculation taking into account a set of features and the current pattern on the board. For one of these moves, the program simulates the game until the end, using a combination of both knowledge and randomness. The score of the simulated game is backed up to the root of the game tree, and the next iteration starts by selecting the next move to evaluate, being the one with the current highest upper confidence bound.

5. **omnetpp**

   From SPEC2017:

   > The benchmark performs discrete event simulation of a large 10 gigabit Ethernet network. The simulation is based on the OMNeT++ discrete event simulation system , a generic and open simulation framework. OMNeT++'s primary application area is the simulation of communication networks, but its generic and flexible architecture allows for its use in other areas such as the simulation of IT systems, queueing networks, hardware architectures or business processes as well.



# Getting Started

To compile and execute these branch predictors, you may use the following instruction:

*(Similiar to the instruction we used in HW3)*

`cd /path/to/your/directory`

`make`

`$PIN_ROOT/pin -t obj-intel64/hw3.so -o outfile.out <parameters>  -- <benchmarks and their inputs> `



## Parameters for each predictor

*(The value shown here is the default value)*

### Two Level Adaptive Predictor

`-hhrtSize 8`

`-ptSize 4`

### GSHARE Predictor

`-ghrSize 8`

### Tournament Predictor

`-ghrSize 8`

`-hhrtSize 8`

`-ptSize 4`

`-chooserSize 4`

### Perceptron Predictor

`-ghrSize 8`

`-pertSize 4`





