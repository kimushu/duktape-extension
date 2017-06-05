declare function describe(description: string, callback: Function): void;
declare function it(expectation: string, callback: Function): void;

interface assert {
	(expression: boolean, message: string): void;
	isOk: (value: any, message?: string) => void;
	isNotOk: (value: any, message?: string) => void;
	exists: (value: any, message?: string) => void;
	notExists: (value: any, message?: string) => void;
	isFunction: (value: any, message?: string) => void;
	isNotFunction: (value: any, message?: string) => void;
	instanceOf: (value: any, constructor: Function, message?: string) => void;
	notInstanceOf: (value: any, constructor: Function, message?: string) => void;
	isUndefined: (value: any, message?: string) => void;
	isDefined: (value: any, message?: string) => void;
	"throws": (fn: Function, errorLike?: Function, errMsgMatcher?: string, message?: string) => void;
	doesNotThrow: (fn: Function, errorLike?: Function, errMsgMatcher?: string, message?: string) => void;
}
declare var assert: assert;

