/// <reference path="./dux_peridot.d.ts"/>
/// <reference path="../hw/dux_paraio.d.ts"/>

declare namespace Dux {
    interface PeridotGPIOConstructor {
        /**
         * Construct a new GPIO instance for given pin (or pins)
         */
        new (begin: PeridotPin, width?: number): PeridotGPIO;
    }

    interface PeridotGPIO extends ParallelIO {
    }
}

declare module "peridot" {
    const GPIO: Dux.PeridotGPIOConstructor;
    type GPIO = Dux.PeridotGPIO;
}
