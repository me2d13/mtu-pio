#!/bin/bash

trim=5000
while [ $trim -le 7000 ]
do
    fractional="${trim:0:-3}.${trim: -3}"
    #echo $fractional
    #echo -n "{\"laminar/B738/flight_model/stab_trim_units\": $fractional}"
    echo -n "{\"laminar/B738/flight_model/stab_trim_units\": $fractional}" | ncat -4u 192.168.1.112 49152
    trim=$(( $trim + 100))
    sleep 0.5
done