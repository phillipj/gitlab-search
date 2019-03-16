/**
 * This internal ReasonML module is used to wrap the 3rd party commander.js package
 * and mostly consists of interop with JavaScript via bucklescript bindings.
 *
 * Useful resources to learn about ReasonML/bucklescript's interop with JavaScript:
 *  - https://medium.com/@Hehk/binding-a-library-in-reasonml-e33b6a58b1b3
 *  - https://github.com/glennsl/bucklescript-ffi-cheatsheet
 *  - https://bucklescript.github.io/bucklescript/Manual.html#_binding_to_a_value_from_a_module_code_bs_module_code
 */

/* t == commander.js being used for commander during its execution */
type t;
type actionFnOptions;

/* this basically does a require("commander") behind the scenes and assigns the result to the type t */
[@bs.module] external commander: t = "";

/* an idiomatic make() function to get a hold of commander.js */
let make = () => commander;

[@bs.send.pipe: t]
external action: (unit /* in reality variadic arguments */ => unit) => t = "";

/**
  This overrides the Commander.action() function for users of this module, needed because
  commander.js invokes the callback provided with variadic (read dynamic) arguments when invoking it.
  That doesn't play well with a strongly typed language like ReasonML, where every argument into a
  function has to be explicitly declared.

  Therefore doing some raw bucklescript tricks here, to convert all the dynamic arguments provided by
  commander.js into *one* array of strings before we invoke the callback provided by the user of this
  Commander ReasonML module.

  Worth mentioning [@bs.variadic] does something similar, but as far as I've understood that's only
  viable for method calls done from ReasonML / our application code. This is the other way around though,
  as this is for making arguments provided *from* JavaScript to ReasonML.
  */
let action: ((array(string), actionFnOptions) => unit, t) => t =
  (fn, t) => {
    // action() below means the external action() definition above, in other words we're not calling
    // ourselfs in a recursive manner here
    action(
      () => {
        // Handling arguments of a callback invoked by JavaScript with variadic arguments isn't
        // straight forward to handling in ReasonML/OCaml's type system. The trick we do below is
        // to grab all the arguments provided by JavaScript, extract the latest argument and treat it
        // as "options", then put all preceding arguments into a string array
        let arguments = [%bs.raw {| arguments |}];
        let argumentsCount = Js.Array.length(arguments);
        let options: actionFnOptions =
          Js.Array.unsafe_get(arguments, argumentsCount - 1);
        let allArgsExceptLast: array(string) =
          Belt.Array.slice(arguments, ~offset=0, ~len=argumentsCount - 1);

        // this it what ends up invoking the callback provided by the code using Commander.action()
        fn(allArgsExceptLast, options);
      },
      t,
    );
  };

// bs.get gets a specific field from a JavaScript object
[@bs.get] external getArgs: t => array(string) = "args";
// bs.get_index gets a user specified field from a JavaScript object, string argument decides which field to get
[@bs.get_index]
external getOption: (actionFnOptions, string) => option(string) = "";

[@bs.send.pipe: t] external arguments: string => t = "";
[@bs.send.pipe: t] external command: string => t = "";
[@bs.send.pipe: t] external description: string => t = "";
[@bs.send.pipe: t] external help: unit = "";
[@bs.send.pipe: t] external option: (string, string) => t = "";
[@bs.send.pipe: t]
external optionWithDefault: (string, string, string) => t = "option";
[@bs.send.pipe: t] external parse: array(string) => t = "";
[@bs.send.pipe: t] external version: string => t = "";