declare module "delay" {
    export = delay;

    function delay(milliseconds: number): Promise<void>;
    function delay<T>(milliseconds: number, value?: T): Promise<T>;

    namespace delay {
        function reject(milliseconds: number, reason?: any): Promise<never>;
    }
}
