describe("Modules", () => {
    it("has global require() function", () => {
        let global: {require: Dux.RequireFunction} = (function(){ return this; })();
        assert.isFunction(global.require);
    });
    it("has global module object <repl>", () => {
        let global: {module: Dux.Module} = (function(){ return this; })();
        assert.exists(global.module);
        assert.equal(global.module.id, "<repl>");
        assert.isUndefined(global.module.parent);
        assert.isNull(global.module.filename);
    });
    it("can load core module", () => {
        let mod: Dux.Console = require("console");
        assert.exists(mod);
    });
    it("can load module (with .js extension)", () => {
        let mod = require("/mod1.js");
        assert.equal(mod.self, "mod1.js");
    });
    it("can load JSON object (with .json extension)", () => {
        let obj = require("/mod1.json");
        assert.deepEqual(obj, {self: "mod1.json"});
    });
    it("can load module (without .js extension)", () => {
        let mod = require("/mod1");
        assert.equal(mod.self, "mod1");
    });
    it("can load module (.js fallback)", () => {
        let mod = require("/mod2");
        assert.equal(mod.self, "mod2.js");
    });
    it("can load JSON object (.json fallback)", () => {
        let obj = require("/json1");
        assert.deepEqual(obj, {self: "json1.json"});
    });
    it("can load nested module with relative path", () => {
        let mod = require("/sub/mod3");
        assert.equal(mod.self, "mod3.js");
        assert.equal(mod.sub.self, "mod2.js");
    });
    it("throws SyntaxError when module has a syntax error", () => {
        assert.throws(() => require("/mod4"), SyntaxError);
    });
    it("caches loaded module", () => {
        let mod_a = require("/mod1.js");
        mod_a.modified = true;
        let mod_b = require("/sub/../mod1.js");
        assert.equal(mod_b.modified, true);
    });
    it("throws Error when module not found", () => {
        assert.throws(() => require("/not_exists"), Error);
    });

    const DUK_EXEC_SUCCESS = 0;
    const DUK_EXEC_ERROR = 1;
    interface EvalModCallerResult {
        result: number; // DUK_EXEC_SUCCESS | DUK_EXEC_ERROR
        retval: any;    // return value
        pushed: number; // number of pushed value (0 or 1)
    }
    let eval_mod_caller: (obj: {}, type: number, data: string, len?: number) => EvalModCallerResult;
    eval_mod_caller = (function(){return this})().__eval_mod_caller;

    describe("dux_eval_module_file()", () => {
        it("succeeds for existing file", () => {
            let o = eval_mod_caller({}, 0, "/dummy/../mod1.js");
            assert.equal(o.pushed, 1);
            assert.notInstanceOf(o.retval, Error);
        });
        it("throws Error for non-existing file", () => {
            assert.throws(() => eval_mod_caller({}, 0, "/non-existing"), Error);
        });
        it("succeeds relative load", () => {
            let o = eval_mod_caller({}, 0, "/sub/mod3.js");
            assert.equal(o.pushed, 1);
            assert.notInstanceOf(o.retval, Error);
        });
    });
    describe("dux_eval_module_file_noresult()", () => {
        it("succeeds for existing file", () => {
            let o = eval_mod_caller({}, 1, "/dummy/../mod1.js");
            assert.equal(o.pushed, 0);
        });
        it("throws Error for non-existing file", () => {
            assert.throws(() => eval_mod_caller({}, 1, "/non-existing"), Error);
        });
    });
    describe("dux_peval_module_file()", () => {
        it("returns DUK_EXEC_SUCCESS for existing file", () => {
            let o = eval_mod_caller({}, 10, "/dummy/../mod1.js");
            assert.equal(o.result, DUK_EXEC_SUCCESS);
            assert.equal(o.pushed, 1);
            assert.notInstanceOf(o.retval, Error);
        });
        it("return DUK_EXEC_ERROR for non-existing file", () => {
            let o = eval_mod_caller({}, 10, "/non-existing");
            assert.equal(o.result, DUK_EXEC_ERROR);
            assert.equal(o.pushed, 1);
            assert.instanceOf(o.retval, Error);
        });
    });
    describe("dux_peval_module_file_noresult()", () => {
        it("returns DUK_EXEC_SUCCESS for existing file", () => {
            let o = eval_mod_caller({}, 11, "/dummy/../mod1.js");
            assert.equal(o.result, DUK_EXEC_SUCCESS);
            assert.equal(o.pushed, 0);
        });
        it("return DUK_EXEC_ERROR for non-existing file", () => {
            let o = eval_mod_caller({}, 11, "/non-existing");
            assert.equal(o.result, DUK_EXEC_ERROR);
            assert.equal(o.pushed, 0);
        });
    });
});
