# **`SRED & ESRED : Active Queue Management in ns3`**

# **`Implementing SRED & ESRED in ns3`**

This repository describes the methodology to implement `Stabilized RED (SRED)`
Algorithm in ns3. Like `RED (Random Early Detection)` SRED pre-emptively
discards packets with a load-dependent probability when a buffer in a router in
the Internet or an Intranet seem congested. SRED has an additional feature
that over a wide range of load levels helps it stabilize its buffer occupation at a level independent of the number of active connections.

We also implement an extended version of SRED where we also consider timestamp of the incoming packets in our algorithm and adjust the probability to
overwrite accordingly. We call this `Extended Stabilized RED` or `ESRED` in short.

## `SRED`
- [Implementation](sred/stabilized-red-queue-disc.cc)
- [Simulation](simulation/wired-sred-simulation.cc)
## `ESRED`
- [Implementation](esred/es-red-queue-disc.cc)
- [Simulation](simulation/wired-esred-simulation.cc)


# **`Task A : Wireless high-rate (mobile)`**

## **Topology**

![](drawings/wireless-jpg/wireless-high-rate.jpg)


## **Simulation**

- [Code](wireless-high-rate/wireless-high-rate.cc)
- [Parameter Variation Graphs](drawings/wifi)

## **Sample Graph Outputs**

![](drawings/wifi/node/drop.jpg)

![](drawings/wifi/node/delivery.jpg)



# **`Task A : Wireless low-rate (static)`**


## **Topology**

![](drawings/wireless-jpg/wireless-lrwpan.jpg)


## **Simulation**

- [Code](wireless-low-rate/wireless-low-rate.cc)
- [Parameter Variation Graphs](drawings/lrpwan)

![](drawings/lrwpan/node/drop.jpg)

![](drawings/lrwpan/node/delivery.jpg)