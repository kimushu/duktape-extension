declare class PromiseSimplified<T> {
    constructor(executor: (resolve: (value?: T) => void, reject: (reason?: any) => void) => void);

    then<TResult1 = any, TResult2 = never>(onFulfilled?: (value: any) => any, onRejected?: (reason: any) => any);
}
export {PromiseSimplified as Promise};
