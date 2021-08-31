# Unable to run the 'circle.ng' example
Jogging under UGS worked, but running the [example code](https://diymachining.com/g-code-example/) resulted in various errors which looked like (number of those messages was also changing from run to run):

```
[Error] An error was detected while sending 'G02X0.Y0.5I0.5J0.F2.5': (error:33) Motion command target is invalid. Streaming has been paused.
**** The communicator has been paused ****

**** Pausing file transfer. ****
```

Building the source with -O2 resolved the issue, so clearly there is some problem with inefficient UART communication.
