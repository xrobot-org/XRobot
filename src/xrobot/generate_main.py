"""生成 User/xrobot_main.hpp：由一份应用配置、入口源文件的 XR_REGISTER 和锁定模块的源码，
得到按顺序静态构造模块实例的 C++ 应用。
Generate User/xrobot_main.hpp: an ordered, static C++ application built from one
application configuration, the XR_REGISTER list of the BSP entry and the locked Module
sources.
"""

from __future__ import annotations

import bisect
import os
import re
from pathlib import Path

from xr_syntax.i18n import tr

from xrobot.config import IDENTIFIER, ConfigError, identifier_problem, load_config, value_text
from xrobot.constructor_model import (
    ValueChecker,
    binding_candidates,
    constructor_for,
    convert,
    initializer_tree,
    is_dependency,
    qualify,
    template_bindings,
    type_shape,
)
from xrobot.module_parser import discover_modules, module_interface, select_module
from xrobot.project import HEADER_NOTICE, Project, atomic_write, read_header_inputs
from xrobot.source_syntax import (
    Token,
    close_token,
    code_tokens,
    conditional_depth,
    parse_document,
    split_arguments,
)
from xrobot.type_index import TypeIndex, module_headers

HELPERS = """namespace xrobot_generated {
// Implicit conversion of a configuration value to an arithmetic parameter type;
// constant conversions keep the compiler's value-change warnings.
template <typename P>
constexpr P Implicit(std::type_identity_t<P> value)
{
  return value;
}
}  // namespace xrobot_generated
"""

# FAT 的时间戳精度是 2 秒；头文件至少比最新的输入晚这么多，才不会与输入落在同一时刻。
# FAT timestamps have a 2-second resolution; the header is at least this much newer than
# its newest input, so the two never share a timestamp.
FRESHNESS_MARGIN_NS = 2_000_000_000


def caller_defined_names(items: list[Token], stop: int) -> set[str]:
    """入口源文件在 stop 之前声明的名字（保守地收集，不做作用域和类型解析）。
    The names the entry source declares before stop, collected conservatively without
    C++ scope or type resolution.

    生成的头文件可能在这些声明之前被包含，甚至在命名空间作用域；注册类型用到这些名字时，
    XRobotMain 把它作为模板参数，由调用处给出。多一个模板参数比看不见的类型名更安全。
    The generated header can be included before these declarations, even at namespace
    scope; a registered type that uses such a name becomes a template parameter of
    XRobotMain, supplied by the call site. An extra template parameter is safer than an
    invisible type name.
    """
    names = set()
    depth = 0
    for i, token in enumerate(items[:stop]):
        text = token.text
        if text == "{":
            if depth and i and items[i - 1].kind == "identifier":
                names.add(items[i - 1].text)  # 包括 constexpr N{2} / includes constexpr N{2}
            depth += 1
        elif text == "}":
            depth = max(0, depth - 1)
        elif text == "=" and i and items[i - 1].kind == "identifier":
            # 包括枚举项和数组长度。
            # Includes enumerators and array extents.
            names.add(items[i - 1].text)
        elif text in ("class", "struct", "enum", "typename") and i + 1 < stop:
            j = i + 1
            if items[j].text in ("class", "struct") and j + 1 < stop:
                j += 1
            if items[j].kind == "identifier":
                names.add(items[j].text)
            if text == "enum":
                while j < stop and items[j].text not in ("{", ";"):
                    j += 1
                if j < stop and items[j].text == "{":
                    end = close_token(items, j)
                    for k in range(j + 1, min(end, stop)):
                        if items[k].kind == "identifier" and items[k - 1].text in ("{", ","):
                            names.add(items[k].text)
        elif text == "template" and i + 1 < stop and items[i + 1].text == "<":
            end = close_token(items, i + 1)
            parameters = " ".join(t.text for t in items[i + 2 : end])
            for parameter in split_arguments(parameters):
                ts = code_tokens(parameter)
                equal = next((j for j, t in enumerate(ts) if t.text == "="), len(ts))
                if equal and ts[equal - 1].kind == "identifier":
                    names.add(ts[equal - 1].text)
        elif text == "using" and i + 1 < stop:
            if items[i + 1].text == "namespace":
                if depth:
                    names.add("*")
            else:
                j = i + 1
                while j + 2 < stop and items[j + 1].text == "::":
                    j += 2
                if items[j].kind == "identifier":
                    names.add(items[j].text)
        elif text == "typedef":
            j = i + 1
            while j < stop and items[j].text != ";":
                if items[j].text in ("{", "(", "[", "<"):
                    j = close_token(items, j) + 1
                else:
                    j += 1
            names.update(
                t.text
                for t in items[i + 1 : j]
                if t.kind == "identifier"
                and t.text
                not in (
                    "void",
                    "bool",
                    "char",
                    "short",
                    "int",
                    "long",
                    "float",
                    "double",
                    "signed",
                    "unsigned",
                    "const",
                    "volatile",
                )
            )
    return names


def read_registrations(path: str | Path) -> list[dict]:
    """读取入口源文件中的 XR_REGISTER(name, Type)，每个名字一个类型。
    Read the XR_REGISTER(name, Type) invocations of the entry source, one type per name.

    Returns:
        每个注册一个映射：name、type、line，以及 caller_view（类型用到入口源文件自己声明
        的名字，XRobotMain 须把它作为模板参数）。
        One mapping per registration: name, type, line and caller_view (the type uses a
        name the entry declares itself, so XRobotMain takes it as a template parameter).

    Raises:
        ConfigError: 写在预处理指令或 #if 中、参数个数不对、名字不合法或重复，或注册了
            引用类型；全部问题一次列出。
            An invocation sits in a directive or under #if, has the wrong number of
            arguments, uses an invalid or repeated name, or registers a reference type;
            every problem is listed at once.
    """
    path = Path(path)
    text = path.read_text(encoding="utf-8-sig", errors="surrogateescape")
    label = path.name
    for number, line in enumerate(text.splitlines(), 1):
        if line.lstrip().startswith("#") and re.search(r"\bXR_REGISTER\b", line):
            raise ConfigError(
                tr(
                    f"{label}:{number}: XR_REGISTER inside a preprocessor directive is not "
                    "supported",
                    f"{label}:{number}: 不支持在预处理指令中写 XR_REGISTER",
                )
            )
    document = parse_document(text, str(path))
    invocations = document.invocation_views("XR_REGISTER", template_angles=True)
    candidates = [
        o
        for o in document.identifier_occurrences()
        if o.text == "XR_REGISTER" and o.following == "("
    ]
    if len(invocations) != len(candidates):
        raise ConfigError(
            tr(f"{label}: malformed XR_REGISTER invocation", f"{label}: XR_REGISTER 的写法不正确")
        )
    tokens = document.code_tokens()
    byte_starts = [token.span.start for token in tokens]
    records, errors, names = [], [], set()
    for invocation in invocations:
        where = f"{label}:{invocation.line}"
        if conditional_depth(document, 0, invocation.span.start):
            errors.append(
                tr(
                    f"{where}: XR_REGISTER inside #if/#ifdef/#ifndef is not supported; the "
                    "generator cannot evaluate build options",
                    f"{where}: 不支持在 #if/#ifdef/#ifndef 中写 XR_REGISTER；"
                    "生成器无法判断编译选项",
                )
            )
            continue
        parts = [p.strip() for p in invocation.arguments]
        if len(parts) != 2:
            errors.append(
                tr(
                    f"{where}: XR_REGISTER registers one type per name: XR_REGISTER(name, Type). "
                    "To expose the object as another type, declare a reference (e.g. "
                    "`LibXR::CAN& can1 = fdcan1;`) and register that name separately",
                    f"{where}: 一个 XR_REGISTER 只登记一个名字和一个类型：XR_REGISTER(name, Type)。"
                    "要以另一种类型提供这个对象，请声明一个引用（例如 "
                    "`LibXR::CAN& can1 = fdcan1;`）并单独登记它",
                )
            )
            continue
        name, cpp_type = parts
        problem = identifier_problem(name)
        if problem:
            errors.append(
                tr(
                    f"{where}: registration name {name} {problem}",
                    f"{where}: 登记名 {name} {problem}",
                )
            )
            continue
        if name in names:
            errors.append(
                tr(
                    f"{where}: duplicate XR_REGISTER name {name}",
                    f"{where}: 重复的 XR_REGISTER 名字 {name}",
                )
            )
            continue
        if cpp_type.endswith("&"):
            errors.append(
                tr(
                    f"{where}: register object types, not reference types: {name}",
                    f"{where}: 请登记对象类型，不要登记引用类型：{name}",
                )
            )
            continue
        names.add(name)
        stop = bisect.bisect_left(byte_starts, invocation.span.start)
        local_names = caller_defined_names(tokens, stop)
        identifiers = {t.text for t in code_tokens(cpp_type) if t.kind == "identifier"}
        records.append(
            {
                "name": name,
                "type": cpp_type,
                "line": invocation.line,
                "caller_view": bool(identifiers & local_names)
                or "decltype" in identifiers
                or "*" in local_names,
            }
        )
    if errors:
        raise ConfigError("\n".join(errors))
    return records


class Generator:
    """把一份配置渲染成生成头文件；错误收集后一次报出。
    Render one configuration as the generated header; errors are collected and reported
    at once.
    """

    def __init__(self, modules: dict, index: TypeIndex | None = None) -> None:
        """modules 是锁定的模块记录；index 缺省时由这些模块的头文件建立。
        modules are the locked Module records; index defaults to one built from their headers.
        """
        self.modules = modules
        self.index = index or TypeIndex.for_modules(modules)
        self.checker = ValueChecker(self.index)
        self._module_classes = {m["name"] for m in self.modules.values()}

    def is_class_name(self, name: str) -> bool:
        """name 是否是模块类名或已加载头文件中的全局类名。
        Whether name is a Module class name or a global class name in the loaded headers.
        """
        return name in self._module_classes or self.index.is_global_class(name)

    def render(
        self,
        config: dict,
        registrations: list[dict],
        source: str,
        config_path: Path | None = None,
        header_path: Path | None = None,
        header_lines: tuple[str, ...] | list[str] = (),
        compile_check: bool = False,
    ) -> str:
        """一份配置的生成头文件文本。
        The text of the generated header for one configuration.

        Args:
            source: 报错时配置的名字。
                The name of the configuration in error messages.
            config_path: 配置文件；与 header_path 都给出时生成 #line 指令。
                The configuration file; with header_path, #line directives are emitted.
            header_lines: 说明行之后的 // xrobot: 输入清单。
                The // xrobot: input list after the notice line.
            compile_check: 生成模块 CI 的编译探针，而不是应用入口。
                Emit the Module CI compile probe instead of the application entry.

        Raises:
            ConfigError: 配置有错；每行一条，带配置名前缀。
                The configuration has errors; one per line, prefixed with its name.
        """
        errors = []

        def fail(message: str) -> None:
            """记下一条以配置名开头的错误。
            Record one error prefixed with the configuration's name.
            """
            errors.append(f"{source}: {message}")

        known = {}
        for record in registrations:
            if self.is_class_name(record["name"]):
                fail(
                    tr(
                        f"XR_REGISTER name {record['name']} is also a Module class name; rename the "
                        "object",
                        f"XR_REGISTER 名字 {record['name']} 同时也是模块的类名；请给对象改名",
                    )
                )
            known[record["name"]] = record["type"]
        entries_config = config.get("modules", [])
        ids = [entry.get("id") for entry in entries_config]
        selected, entries, monitored, earlier = {}, [], set(), {}
        for i, entry in enumerate(entries_config):
            identity = entry["id"]
            try:
                result = self._instance(i, entry, ids, known, earlier, selected, compile_check)
            except ValueError as error:
                for line in str(error).splitlines():
                    fail(line if line.startswith(identity) else f"{identity}: {line}")
                earlier[identity] = None
                continue
            entries.append(result)
            earlier[identity] = result["cpp_type"]
            if result["monitor"]:
                monitored.add(identity)
        constants = []
        namespace = config.get("constexpr_namespace", "ProjectConstexpr")
        for name, spec in config.get("constexprs", {}).items():
            try:
                cpp_type = value_text(spec["type"], f"constexprs.{name}.type")
                self.checker.checks = []
                expr, typed = self.checker.render(
                    spec["value"], "constexprs." + name, cpp_type, (), None
                )
                if expr.lstrip().startswith("{"):
                    expr = cpp_type + expr
                constants.append(f"inline constexpr {cpp_type} {name} = {expr};")
            except ValueError as error:
                fail(str(error))
        if errors:
            raise ConfigError("\n".join(errors))
        return self._assemble(
            config,
            registrations,
            entries,
            monitored,
            constants,
            namespace,
            config_path,
            header_path,
            header_lines,
            compile_check,
            selected,
        )

    def _instance(
        self,
        i: int,
        entry: dict,
        ids: list[str],
        known: dict[str, str],
        earlier: dict[str, str | None],
        selected: dict[str, dict],
        compile_check: bool,
    ) -> dict:
        """渲染一个实例：选构造函数、渲染每个参数、判断是否调用 OnMonitor。
        Render one instance: select its constructor, render each argument and decide
        whether its OnMonitor is called.

        Args:
            known: 注册名到类型的映射。
                The types of the registration names.
            earlier: 排在前面的实例 id 到 C++ 类型的映射；有错的实例为 None。
                The C++ types of earlier instance ids; None for an instance with errors.
            selected: 已选中的模块，按类名；本实例的模块会加入其中。
                The selected Modules by class name; this instance's Module is added.

        Raises:
            ValueError: 实例有错；每行一条。
                The instance has errors; one per line.
        """
        identity = entry["id"]
        if identity in known:
            raise ValueError(
                tr(
                    f"instance id {identity} is also an XR_REGISTER name",
                    f"实例 id {identity} 同时也是 XR_REGISTER 名字",
                )
            )
        if self.is_class_name(identity):
            raise ValueError(
                tr(
                    f"instance id {identity} is also a class name in the loaded Modules; use a "
                    f"lower-case id such as {identity.lower()}",
                    f"实例 id {identity} 同时也是已加载模块中的类名；请用小写的 id，例如 "
                    f"{identity.lower()}",
                )
            )
        module = select_module(self.modules, entry["module"])
        if not module["manifest"].standalone:
            raise ValueError(
                tr(
                    f"{module['id']} is a library (standalone: false) and cannot be instantiated",
                    f"{module['id']} 是库（standalone: false），不能创建实例",
                )
            )
        if module["name"] in selected and selected[module["name"]]["id"] != module["id"]:
            first = selected[module["name"]]["id"]
            raise ValueError(
                tr(
                    f"{first} and {module['id']} both define the global class {module['name']}; "
                    "use only one of them",
                    f"{first} 和 {module['id']} 都定义了全局类 {module['name']}；只能使用其中一个",
                )
            )
        selected[module["name"]] = module
        interface = module_interface(module)
        template_args = [
            value_text(v, f"{identity}.template_args[{j}]")
            for j, v in enumerate(entry.get("template_args", []))
        ]
        cpp_type = module["name"]
        if template_args or interface["template_parameters"]:
            cpp_type += "<" + ", ".join(template_args) + ">"
        templates = template_bindings(interface, template_args)
        named_values = entry.get("args", [])
        visible = dict(known)
        visible.update({k: v for k, v in earlier.items() if v is not None})
        later = set(ids[i:])
        ctor = constructor_for(interface, named_values, visible, cpp_type, templates)
        args_lines = getattr(entry.get("args"), "item_lines", None) or []
        declarations, arguments, problems = [], [], []
        for j, (p, item) in enumerate(zip(ctor["arguments"], named_values, strict=False)):
            value = next(iter(item.values()))
            line = args_lines[j] if j < len(args_lines) else getattr(entry, "line", 0)
            try:
                decl, arg = self._argument(
                    identity,
                    p,
                    value,
                    interface,
                    cpp_type,
                    templates,
                    visible,
                    later,
                    earlier,
                    compile_check,
                )
            except ValueError as error:
                problems.append(str(error))
                continue
            declarations.append((line, decl))
            arguments.append((line, arg))
        located = self.index.resolve(module["name"])
        monitor = self.index.provides_monitor(located) if located is not None else None
        if monitor is None:
            problems.append(
                tr(
                    f"{identity}: cannot tell whether a public base class provides OnMonitor; its "
                    "base is not defined in the loaded Module headers",
                    f"{identity}: 无法判断公有基类是否提供 OnMonitor；"
                    "它的基类不在已加载的模块头文件中",
                )
            )
        if problems:
            raise ValueError("\n".join(problems))
        return {
            "id": identity,
            "cpp_type": cpp_type,
            "arguments": arguments,
            "index": i,
            "declarations": declarations,
            "monitor": monitor,
            "line": getattr(entry, "line", 0),
        }

    def _argument(
        self,
        identity: str,
        p: dict,
        value: object,
        interface: dict,
        cpp_class: str,
        templates: dict[str, str],
        visible: dict[str, str],
        later: set[str],
        earlier: dict[str, str | None],
        compile_check: bool,
    ) -> tuple[list[str], str]:
        """渲染一个构造参数。
        Render one constructor argument.

        依赖按名字绑定到注册名或排在前面的实例；值交给 ValueChecker 和 convert。引用参数和
        std::initializer_list 的值放进 static 变量，使其活得和实例一样久。
        A dependency binds by name to a registration or an earlier instance; a value goes
        through ValueChecker and convert. Values for reference and std::initializer_list
        parameters live in static variables, as long as the instance.

        Returns:
            (实例之前要写出的声明, 参数表达式)。
            (declarations to emit before the instance, argument expression).

        Raises:
            ValueError: 名字无法绑定或值不符合规则。
                A name cannot be bound or a value breaks a rule.
        """
        field = f"{identity}.args.{p['name']}"
        typ = qualify(p["type"], interface, cpp_class, templates)
        target = qualify(p["type"], interface, cpp_class, templates, True)
        _, _, pointers, reference = type_shape(target)
        checks = []
        if value is None and compile_check and p["default"] is None:
            raw = f"*static_cast<std::remove_reference_t<{typ}>*>(xr_ci_null)"
            return [], (f"static_cast<{typ}>({raw})" if typ.rstrip().endswith("&&") else raw)
        default = None
        if p["default"] is not None:
            default = initializer_tree(
                qualify(p["default"], interface, cpp_class, templates), target
            )
        self.checker.checks = checks
        if isinstance(value, str):
            text = value_text(value, field)
            name = text.strip()
            reference_name = name[1:].strip() if name.startswith("&") else name
            if re.fullmatch(IDENTIFIER, reference_name) and (
                reference_name == name or len(pointers) == 1
            ):
                if reference_name == identity:
                    raise ValueError(
                        tr(
                            f"{field} refers to {identity} itself",
                            f"{field} 引用了 {identity} 自身",
                        )
                    )
                if reference_name in later:
                    raise ValueError(
                        tr(
                            f"{field}: {reference_name} is constructed after {identity}; "
                            "instances are constructed in list order",
                            f"{field}: {reference_name} 在 {identity} 之后才构造；"
                            "实例按列表顺序构造",
                        )
                    )
                if reference_name in earlier and earlier[reference_name] is None:
                    raise ValueError(
                        tr(
                            f"{field}: {reference_name} has errors of its own",
                            f"{field}: {reference_name} 本身有错误",
                        )
                    )
                if is_dependency(p) and reference_name not in visible and name != "nullptr":
                    candidates = self._candidates(target, visible)
                    raise ValueError(
                        tr(
                            f"{field}: {reference_name} is neither an XR_REGISTER name nor an "
                            f"earlier instance id; candidates of type {target}: "
                            f"{', '.join(candidates) or 'none'}",
                            f"{field}: {reference_name} 既不是 XR_REGISTER 名字，也不是前面实例的 "
                            f"id；类型为 {target} 的候选：{'、'.join(candidates) or '无'}",
                        )
                    )
                if reference_name in visible:
                    source = visible[reference_name]
                    expression = name
                    if name.startswith("&"):
                        source += "*"
                    elif len(pointers) == 1 and not type_shape(source)[2] and reference != "&":
                        # 指针参数写对象名时传对象地址。
                        # A bare name bound to a pointer parameter passes the object's address.
                        expression = f"std::addressof({name})"
                        source += "*"
                    if self._certainly_unrelated(source, target):
                        raise ValueError(
                            tr(
                                f"{field}: {reference_name} is a {visible[reference_name]}, which "
                                f"does not convert to {target}",
                                f"{field}: {reference_name} 的类型是 {visible[reference_name]}，"
                                f"不能转换为 {target}",
                            )
                        )
                    if type_shape(source)[0] == type_shape(target)[0] and len(
                        type_shape(source)[2]
                    ) == len(pointers):
                        return checks, expression  # 同一类型直接绑定 / the same type binds directly
                    if reference:
                        convert(expression, typ, False, checks, field)
                        return checks, f"static_cast<{typ}>({expression})"
                    expr, _ = convert(expression, typ, False, checks, field)
                    return checks, expr
            expr, typed = self.checker.render(text, field, target, (), default)
        else:
            expr, typed = self.checker.render(value, field, target, (), default)
        exact = typed or isinstance(value, (dict, list))
        if reference or "std::initializer_list<" in target.replace(" ", ""):
            # 配置里的临时值和 initializer_list 的底层数组需要静态生存期。
            # Config temporaries and initializer-list backing arrays need static lifetime.
            storage = f"xr_arg_{identity}_{p['name']}"
            checks.append(f"static {typ} {storage} =\n      {expr}\n  ;")
            return checks, (
                f"static_cast<{typ}>({storage})" if typ.rstrip().endswith("&&") else storage
            )
        expr, _ = convert(expr, typ, exact, checks, field)
        return checks, expr

    def _candidates(self, target: str, visible: dict[str, str]) -> list[str]:
        """可绑定到 target 的名字，用于报错提示；规则与 describe 给插件的候选相同。
        The names that bind to target, for error hints; the same rule as the candidates
        describe gives the editor.
        """
        return binding_candidates(self.index, target, visible)

    def _certainly_unrelated(self, source: str, target: str) -> bool:
        """只有能确定 source 类型的对象不能转换为 target 时才为 True。
        True only when an object of type source can be shown not to convert to target.
        """
        sb, _, sp, _ = type_shape(source)
        tb, _, tp, _ = type_shape(target)
        if sb == tb or len(sp) != len(tp):
            # 指针的转换由生成器的取地址规则决定。
            # Pointer adaptation is decided by the generator's address-of rule.
            return False
        return self.index.derives_from(sb, tb) is False

    def _assemble(
        self,
        config: dict,
        registrations: list[dict],
        entries: list[dict],
        monitored: set[str],
        constants: list[str],
        namespace: str,
        config_path: Path | None,
        header_path: Path | None,
        header_lines: tuple[str, ...] | list[str],
        compile_check: bool,
        selected: dict[str, dict],
    ) -> str:
        """拼出头文件：输入清单、include、常量、XRobotMain、主循环和两个宏。
        Assemble the header: input list, includes, constants, XRobotMain, the monitor loop
        and the two macros.

        XRobotMain 只接收实例用到的注册；每个实例和参数前的 #line 指回 YAML，实例之后的
        #line 指回头文件。
        XRobotMain takes only the registrations the instances use; a #line before each
        instance and argument points into the YAML, and one after each instance points back
        into the header.
        """
        used = set()
        for entry in entries:
            for expression in (
                [entry["cpp_type"]]
                + [a for _, a in entry["arguments"]]
                + [d for _, ds in entry["declarations"] for d in ds]
            ):
                used.update(t.text for t in code_tokens(expression) if t.kind == "identifier")
        views = [r for r in registrations if r["name"] in used]
        template_names = {
            t.text for r in views for t in code_tokens(r["type"]) if t.kind == "identifier"
        }
        template_names |= set(selected) | {e["id"] for e in entries} | {r["name"] for r in views}
        local_templates, parameters, actual_types = [], [], []
        for i, record in enumerate(views):
            typ = record["type"]
            if record["caller_view"]:
                parameter_type = f"XrViewType{i}"
                while parameter_type in template_names:
                    parameter_type += "_"
                template_names.add(parameter_type)
                local_templates.append("typename " + parameter_type)
                actual_types.append(typ)
                typ = parameter_type
            declaration = (
                f"std::add_lvalue_reference_t<{typ}>"
                if any(t.text in ("(", "[") for t in code_tokens(typ))
                else typ + "&"
            )
            parameters.append(f"{declaration} {record['name']}")
        # 说明在第二行；读取 // xrobot: 行时跳过它（project.read_header_inputs）。
        # The notice is the second line; readers of the // xrobot: lines skip it.
        if compile_check:
            lines = ["// Generated by `xrobot check-module` for Module CI; do not edit by hand."]
        else:
            lines = ["#pragma once", HEADER_NOTICE, *header_lines]
        lines += [
            "",
            "#include <memory>",
            "#include <type_traits>",
            "#include <utility>",
            '#include "libxr.hpp"',
            '#include "thread.hpp"',
        ]
        lines += [f'#include "{name}.hpp"' for name in selected]
        for header in config.get("constexpr_includes", []):
            header = header.strip()
            include = header if header.startswith("<") else f'"{header}"'
            lines.append(f"#include {include}")
        lines += ["", HELPERS]
        if constants:
            lines += (
                [f"namespace {namespace} {{"] + constants + [f"}}  // namespace {namespace}", ""]
            )
        back = object()  # 占位：#line 回到本头文件 / placeholder for "#line back into this header"
        directives = config_path is not None and header_path is not None and not compile_check
        cfg = Path(os.path.abspath(config_path)).as_posix() if directives else None

        def at(line: int) -> None:
            """写出 #line 指令时，让后面的代码指回配置的第 line 行。
            When #line directives are written, point the following code back to line of the
            configuration.
            """
            if directives and line:
                lines.append(f'#line {line} "{cfg}"')

        if compile_check:
            lines += [
                "namespace xrobot_generated {",
                "void XRobotCompileCheck() {",
                "  // Compilation only: this function must never be invoked.",
                "  [[maybe_unused]] static void* xr_ci_null = static_cast<void*>(nullptr);",
            ]
        else:
            if local_templates:
                lines.append(f"template <{', '.join(local_templates)}>")
            signature = "[[noreturn]] static inline void XRobotMain("
            if parameters:
                lines += [signature, "    " + ",\n    ".join(parameters) + ")", "{"]
            else:
                lines += [signature + ")", "{"]
        for entry in entries:
            lines.append(f"  // modules[{entry['index']}]: {entry['id']}")
            for line, declarations in entry["declarations"]:
                if declarations:
                    at(line)
                    lines.extend("  " + d for d in declarations)
            at(entry["line"])
            if entry["arguments"]:
                lines.append(f"  static {entry['cpp_type']} {entry['id']}(")
                for k, (line, argument) in enumerate(entry["arguments"]):
                    at(line)
                    lines.append(f"      {', ' if k else ''}{argument}")
                lines.append("  );")
            else:
                lines.append(f"  static {entry['cpp_type']} {entry['id']};")
            if directives:
                lines.append(back)
        lines += [
            f'  static_assert(std::is_void_v<decltype({e["id"]}.OnMonitor())>, "{e["id"]}.OnMonitor() must return void");'
            for e in entries
            if e["id"] in monitored
        ]
        if compile_check:
            lines += [f"  {e['id']}.OnMonitor();" for e in entries if e["id"] in monitored]
            lines += ["}", "}  // namespace xrobot_generated", ""]
            return "\n".join(lines)
        lines += ["  for (;;)", "  {"]
        lines += [f"    {e['id']}.OnMonitor();" for e in entries if e["id"] in monitored]
        lines += [
            f"    LibXR::Thread::Sleep({config.get('settings', {}).get('monitor_sleep_ms', '1000')});",
            "  }",
            "}",
            "",
        ]
        lines += [
            "// XR_REGISTER marks names for the generator and references the object, so",
            "// registrations the selected product does not consume raise no variable",
            "// warnings; types are checked where XRobotMain binds them.",
            "#define XR_REGISTER(name, ...) static_cast<void>(name)",
        ]
        call = "::XRobotMain" + (f"<{', '.join(actual_types)}>" if actual_types else "")
        arguments = ", ".join(r["name"] for r in views)
        if len(call) + len(arguments) < 72:
            lines.append(f"#define XROBOT_MAIN() {call}({arguments})")
        else:
            lines += ["#define XROBOT_MAIN() \\", f"  {call}( \\"]
            lines += [
                f"      {r['name']}{',' if i + 1 < len(views) else ''} \\"
                for i, r in enumerate(views)
            ]
            lines.append("  )")
        lines.append("")
        if directives:
            own = Path(os.path.abspath(header_path)).as_posix()
            result, number = [], 1  # 下一行输出的行号 / line number of the next emitted line
            for line in lines:
                if line is back:
                    line = f'#line {number + 1} "{own}"'
                result.append(line)
                number += line.count("\n") + 1
            lines = result
        return "\n".join(lines)


def generate_code(
    project: Project,
    config_path: str | Path,
    modules: dict,
    registrations: list[dict],
    index: TypeIndex | None = None,
) -> str:
    """一份配置的生成头文件文本，不写文件。
    The generated header for one configuration, without writing it.

    Raises:
        ConfigError: 配置有错。
            The configuration has errors.
    """
    config_path = Path(config_path)
    source = project.relative(config_path)
    config = load_config(config_path, source)
    depends = (
        ([project.lock] if project.lock.is_file() else [])
        + [project.entry()]
        + sorted(set(module_headers(modules)))
    )
    generator = Generator(modules, index)
    return generator.render(
        config,
        registrations,
        source,
        config_path,
        project.header,
        project.header_lines(config_path, depends),
    )


def load_modules(project: Project) -> dict:
    """BSP 中锁定的全部模块。
    Every Module locked in the BSP.
    """
    return discover_modules(project.modules_dir, project.lock)


def _mark_fresh(project: Project) -> None:
    """把生成头文件的修改时间设在它记录的每个输入之后。
    Give the generated header a modification time after every input it lists.

    通常是当前时间。输入的修改时间在未来（时钟不一致、网络共享、从别的机器拷来），或文件
    系统时间精度粗使两者相同时，改为最新输入之后 FRESHNESS_MARGIN_NS；否则 LibXR 的 CMake
    检查会一直认为头文件过期。
    Normally the current time. When an input's time lies in the future (clock skew, a
    network share, files copied from another machine) or a coarse filesystem clock makes
    the two equal, it becomes FRESHNESS_MARGIN_NS after the newest input; otherwise LibXR's
    CMake check would keep calling the header stale.
    """
    os.utime(project.header)
    config, depends = read_header_inputs(project.header)
    base = project.header.parent
    inputs = [base / p for p in ([config] if config else []) + depends]
    newest = max((p.stat().st_mtime_ns for p in inputs if p.exists()), default=0)
    if project.header.stat().st_mtime_ns <= newest:
        target = newest + FRESHNESS_MARGIN_NS
        os.utime(project.header, ns=(target, target))


def generate(
    project: Project,
    config_path: str | Path | None = None,
    modules: dict | None = None,
    index: TypeIndex | None = None,
) -> str:
    """为 config_path（缺省为当前选中的产品）生成 User/xrobot_main.hpp。
    Generate User/xrobot_main.hpp for config_path (default: the selected product).

    内容不变时不重写，但修改时间仍更新到所有输入之后，使构建的检查认为它是最新的。
    Unchanged content is not rewritten, but the modification time still moves after every
    input so the build's check accepts the header.

    Args:
        modules, index: 调用方已读取的模块和类型索引（setup 检查全部配置时已建好）。
            Modules and type index the caller already has (setup builds them to check
            every configuration).

    Raises:
        ConfigError: 配置不存在或有错。
            The configuration does not exist or has errors.
    """
    config_path = Path(config_path) if config_path else project.selected_config()
    if not config_path.is_file():
        raise ConfigError(
            tr(
                f"{project.relative(config_path)} does not exist",
                f"{project.relative(config_path)} 不存在",
            )
        )
    modules = modules if modules is not None else load_modules(project)
    registrations = read_registrations(project.entry())
    code = generate_code(project, config_path, modules, registrations, index)
    atomic_write(project.header, code)
    _mark_fresh(project)
    return code


def validate_all(
    project: Project, modules: dict | None = None, index: TypeIndex | None = None
) -> int:
    """检查 BSP 的每份应用配置，一次列出全部错误。
    Check every application configuration of the BSP and report every error at once.

    Returns:
        检查的配置数。
        The number of configurations checked.

    Raises:
        ConfigError: 任何配置有错。
            Any configuration has errors.
    """
    modules = modules if modules is not None else load_modules(project)
    index = index or TypeIndex.for_modules(modules)
    registrations = read_registrations(project.entry())
    errors = []
    for config_path in project.configs():
        try:
            generate_code(project, config_path, modules, registrations, index)
        except ValueError as error:
            errors.append(str(error))
    if errors:
        raise ConfigError("\n".join(errors))
    return len(project.configs())


def generate_compile_check(
    module_name: str, modules: dict, output: str | Path, template_args: list[str] | None = None
) -> str:
    """模块 CI 的编译探针：一个从不执行的构造调用，依赖参数用 void* 占位。
    The Module CI compile probe: a constructor call that never runs, with void*
    placeholders for dependencies.

    库模块（standalone: false）没有模块构造函数，探针只包含它的头文件；它的 .cpp 仍经模块
    列表编译进 xr。
    A library (standalone: false) has no Module constructor, so the probe only includes its
    header; its .cpp files still compile into xr through the Module list.
    """
    from xrobot.config_edit import seed_arguments

    module = select_module(modules, module_name)
    if not module["manifest"].standalone:
        code = f'#include "{module["name"]}.hpp"\n'
        atomic_write(Path(output), code)
        return code
    interface = module_interface(module)
    supplied = list(template_args or [])
    templates = template_bindings(interface, supplied)
    cpp_class = module["name"] + (
        "<" + ", ".join(supplied) + ">" if interface["template_parameters"] else ""
    )
    generator = Generator(modules)
    entry = {
        "module": module["id"],
        "id": "module_0",
        "args": seed_arguments(interface, cpp_class, templates, generator.index),
    }
    if supplied:
        entry["template_args"] = supplied
    code = generator.render({"modules": [entry]}, [], module["id"], compile_check=True)
    atomic_write(Path(output), code)
    return code
