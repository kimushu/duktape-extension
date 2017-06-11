/// <reference path="../hw/dux_paraio.d.ts"/>

declare namespace Dux {
    /**
     * Pin specify (both number and string with prefix "D"/"d" are accepted)
     * 
     * Example: 1, "D2", "d3"
     */
    type PeridotPin = number | string;
}

declare module "peridot" {
    export const startLed: Dux.ParallelIO;
}
