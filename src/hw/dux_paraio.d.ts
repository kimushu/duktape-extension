declare namespace Dux {
    interface ParallelIOConstructor {
        readonly prototype: ParallelIO;
    }

    interface ParallelIO {
        /**
         * Assert all bits according to polarity settings
         */
        assert(): ParallelIO;

        /**
         * Negate all bits according to polarity settings
         */
        negate(): ParallelIO;

        /**
         * Set all bits (same as high())
         */
        set(): ParallelIO;

        /**
         * Clear all bits (same as low())
         */
        clear(): ParallelIO;

        /**
         * Change all bits to high (same as set())
         */
        high(): ParallelIO;

        /**
         * Change all bits to low (same as clear())
         */
        low(): ParallelIO;

        /**
         * Toggle each bits (high to low, low to high)
         */
        toggle(): ParallelIO;

        /**
         * Enable output for all bits
         */
        enableOutput(): ParallelIO;

        /**
         * Disable output for all bits
         */
        disableOutput(): ParallelIO;

        /**
         * Enable input for all bits
         */
        enableInput(): ParallelIO;

        /**
         * Disable input for all bits
         */
        disableInput(): ParallelIO;

        /**
         * Set polarity as active high for all bits
         */
        setActiveHigh(): ParallelIO;

        /**
         * Set polarity as active low for all bits
         */
        setActiveLow(): ParallelIO;

        /**
         * Lock direction and polarity
         */
        lock(): ParallelIO;

        /**
         * Unlock direction and polarity
         */
        unlock(): ParallelIO;

        /**
         * Slice one-bit
         */
        [index: number]: ParallelIO;

        /**
         * Slice bits
         * @param begin Offset of LSB
         * @param end Offset of MSB
         */
        slice(begin: number, end?: number): ParallelIO;

        /**
         * Check if all bits are asserted according to polarity settings
         * (true: asserted all / false: negated all / null: others)
         */
        readonly isAsserted: boolean;

        /**
         * Check if all bits are negated according to polarity settings
         * (true: negated all / false: asserted all / null: others)
         */
        readonly isNegated: boolean;

        /**
         * Check if all bits are high (same as isSet)
         * (true: all high / false: all low / null: others)
         */
        readonly isHigh: boolean;

        /**
         * Check if all bits are low (same as isCleared)
         * (true: all low / false: all high / null: others)
         */
        readonly isLow: boolean;

        /**
         * Check if all bits are set (same as isHigh)
         * (true: all set / false: all cleared / null: others)
         */
        readonly isSet: boolean;

        /**
         * Check if all bits are cleared (same as isLow)
         * (true: all cleared / false: all set / null: others)
         */
        readonly isCleared: boolean;

        /**
         * Value
         */
        value: number;
    }
}

declare module "hardware" {
    const ParallelIO: Dux.ParallelIOConstructor;
    type ParallelIO = Dux.ParallelIO;
}
