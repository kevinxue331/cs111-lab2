# You Spin Me Round Robin

Implementation of round robin scheuling in C using an input of a text file and quantum length containing start time and burst length to calculate average waiting and responce time 

## Building

```shell
make
```

## Running

cmd for running
```shell
./rr [input file] [burst length]
```

results 
```shell
./rr processes.txt 3
Average waiting time: 7.00
Average response time: 2.75

```

## Cleaning up

```shell
make clean
```
