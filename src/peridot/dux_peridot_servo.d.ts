/// <reference path="./dux_peridot.d.ts"/>

declare namespace Dux {
    interface PeridotServoConstructor {
        readonly prototype: PeridotServo;

        /**
         * Construct a servo signal generator
         */
        new (pin: PeridotPin): PeridotServo;

        /**
         * Enable all servo signal generator
         */
        enableAll(): void;

        /**
         * Disable all servo signal generator
         */
        disableAll(): void;
    }

    interface PeridotServo {
        /** Raw value */
        rawValue: number;

        /** Pin number */
        readonly pin: number;
    }
}

declare module "peridot" {
    const Servo: Dux.PeridotServoConstructor;
    type Servo = Dux.PeridotServo;
}
