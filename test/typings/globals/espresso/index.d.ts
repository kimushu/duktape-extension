declare function describe(description: string, callback: Function): void;
declare function xdescribe(description: string, callback: Function): void;
declare function it(expectation: string, callback: Function): void;
declare function xit(expectation: string, callback: Function): void;

interface assert {
	(expression: boolean, message: string): void;
	isOk: (value: any, message?: string) => void;
	isNotOk: (value: any, message?: string) => void;
	exists: (value: any, message?: string) => void;
	notExists: (value: any, message?: string) => void;
	isNumber: (value: any, message?: string) => void;
	isNotNumber: (value: any, message?: string) => void;
	isFunction: (value: any, message?: string) => void;
	isNotFunction: (value: any, message?: string) => void;
	isString: (value: any, message?: string) => void;
	isNotString: (value: any, message?: string) => void;
	isTrue: (value: any, message?: string) => void;
	isNotTrue: (value: any, message?: string) => void;
	isFalse: (value: any, message?: string) => void;
	isNotFalse: (value: any, message?: string) => void;
	instanceOf: (value: any, constructor: Function, message?: string) => void;
	notInstanceOf: (value: any, constructor: Function, message?: string) => void;
	isUndefined: (value: any, message?: string) => void;
	isDefined: (value: any, message?: string) => void;
	"throws": (fn: Function, errorLike?: any, errMsgMatcher?: any, message?: string) => void;
	doesNotThrow: (fn: Function, errorLike?: any, errMsgMatcher?: any, message?: string) => void;
	equal: (actual: any, expected: any, message?: string) => void;
	notEqual: (actual: any, expected: any, message?: string) => void;
	strictEqual: (actual: any, expected: any, message?: string) => void;
	notStrictEqual: (actual: any, expected: any, message?: string) => void;
	deepEqual: (actual: any, expected: any, message?: string) => void;
	notDeepEqual: (actual: any, expected: any, message?: string) => void;
	property: (object: any, property: string, message?: string) => void;
	notProperty: (object: any, property: string, message?: string) => void;
	isFulfilled: (promise: any, message?: string) => any;
	isRejected: (promise: any, errorLike?: any, errMsgMathcer?: any, message?: string) => any;
	becomes: (promise: any, value: any, message?: string) => any;
	doesNotBecome: (promise: any, value: any, message?: string) => any;
}
declare var assert: assert;

