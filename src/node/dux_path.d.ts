declare namespace Dux {
    interface PathObject {
        dir?: string;
        root?: string;
        base?: string;
        name?: string;
        ext?: string;
    }

    module Path {
        /**
         * Returns the last portionof a path, similar to the Unix basename command.
         * @param path
         * @param ext An optional file extension
         */
        // function basename(path: string, ext?: string): string;

        /**
         * Provides the platform-specific path delimiter
         */
        const delimiter: string;

        /**
         * Returns directory name of a path, similar to the Unix dirname command.
         * @param path
         */
        function dirname(path: string): string;

        /**
         * Returns the extension of the path, from the last occurence of the . (period) character to end of string.
         * @param path
         */
        // function extname(path: string): string;

        /**
         * Returns path string from an object.
         * @param pathObject
         */
        // function format(pathObject: Dux.PathObject): string;

        /**
         * Determines if path is an absolute path.
         * @param path
         */
        // function isAbsolute(path: string): boolean;

        /**
         * Joins all given path segments together using separator, then normalizes the resulting path.
         * @param paths A sequence of path segments
         */
        // function join(...paths: string[]): string;

        /**
         * Normalizes the given path, resolving '..' and '.' segments.
         * @param path
         */
        function normalize(path: string): string;

        /**
         * Returns an object whose properties represent significant elements of the path.
         * @param path
         */
        // function parse(path: string): Dux.PathObject;

        /**
         * Provides to access POSIX specific implementations (Circular reference to path module itself)
         */
        const posix: typeof Dux.Path;

        /**
         * Returns the relative path from 'from' to 'to'.
         * @param from
         * @param to
         */
        // function relative(from: string, to: string): string;

        /**
         * Resolves a sequence of paths or path segments into an absolute path.
         * @param paths A sequence of paths or path segments
         */
        // function resolve(...paths: string[]): string;

        /**
         * Provides path segment separator (/)
         */
        const sep: string;
    }
}
declare module "path" {
    export = Dux.Path;
}
