"""生成 User/xrobot_main.hpp：由一份应用配置、入口源文件的 XR_REGISTER 和锁定模块的源码，
得到按顺序静态构造模块实例的 C++ 应用。
Generate User/xrobot_main.hpp: an ordered, static C++ application built from one
application configuration, the XR_REGISTER list of the BSP entry and the locked Module
sources.
"""

from __future__ import annotations

import bisect
import re
from pathlib import Path

from xr_syntax.i18n import tr

from xrobot.config import IDENTIFIER, ConfigError, identifier_problem, load_config, value_text
from xrobot.constructor_model import (
    ValueChecker,
    argument_text,
    base_spelling,
    binding_candidates,
    constructor_for,
    initializer_tree,
    is_arithmetic,
    is_dependency,
    qualify,
    template_bindings,
    type_shape,
)
from xrobot.cpp_layout import (
    INDENT,
    layout_call,
    layout_declaration,
    layout_macro,
    layout_value,
    sort_includes,
    wrap_comment,
)
from xrobot.module_parser import discover_modules, module_interface, select_module
from xrobot.project import Project, atomic_write, header_banner, header_relative
from xrobot.source_syntax import (
    Token,
    close_token,
    code_tokens,
    conditional_depth,
    parse_document,
    split_arguments,
    viable_constructors,
)
from xrobot.type_index import TypeIndex, module_headers


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
        line_directives: bool = True,
    ) -> str:
        """一份配置的生成头文件文本。
        The text of the generated header for one configuration.

        Args:
            source: 报错时配置的名字，也是头文件说明行中的配置路径。
                The name of the configuration in error messages and in the notice line of the
                header.
            config_path: 配置文件；与 header_path 都给出时，每个实例前的注释记录它的行号，
                并按 line_directives 生成 #line 指令。
                The configuration file; with header_path, the comment before each instance
                records its line, and #line directives follow line_directives.
            header_lines: 文件末尾的 // xrobot: 输入清单。
                The // xrobot: input list at the end of the file.
            compile_check: 生成模块 CI 的编译探针，而不是应用入口。
                Emit the Module CI compile probe instead of the application entry.
            line_directives: 每个实例前写 #line，使编译错误指回 YAML。
                Write a #line before each instance, so compiler errors point back into the
                YAML.

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
        selected, entries, monitored, earlier, storage_names = {}, [], set(), {}, {}
        for i, entry in enumerate(entries_config):
            identity = entry["id"]
            try:
                result = self._instance(i, entry, ids, known, earlier, selected, compile_check)
            except ValueError as error:
                for line in str(error).splitlines():
                    fail(line if line.startswith(identity) else f"{identity}: {line}")
                earlier[identity] = None
                continue
            for name, origin in result["storage_names"].items():
                if name in storage_names:
                    fail(
                        tr(
                            f"{origin} and {storage_names[name]} would both be stored as {name}; "
                            "rename one of the instance ids",
                            f"{origin} 和 {storage_names[name]} 的存储都会叫 {name}；"
                            "请给其中一个实例改 id",
                        )
                    )
                storage_names[name] = origin
            entries.append(result)
            earlier[identity] = result["cpp_type"]
            if result["monitor"]:
                monitored.add(identity)
        constants = []
        namespace = config.get("constexpr_namespace", "ProjectConstexpr")
        for name, spec in config.get("constexprs", {}).items():
            try:
                cpp_type = value_text(spec["type"], f"constexprs.{name}.type")
                expr, _ = self.checker.render(
                    spec["value"], "constexprs." + name, cpp_type, (), None
                )
                constants.append((cpp_type, name, expr))
            except ValueError as error:
                fail(str(error))
        if errors:
            raise ConfigError("\n".join(errors))
        location = None
        if config_path is not None and header_path is not None and not compile_check:
            location = (header_relative(config_path, header_path), Path(header_path).name)
        return self._assemble(
            config,
            registrations,
            entries,
            monitored,
            constants,
            namespace,
            source,
            location,
            line_directives,
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
        # 同名参数数的构造函数不止一个时，值要按目标类型转换，才由 YAML 的参数名选定构造函数。
        # With several constructors that could take this call, values are converted to their
        # target types so that the YAML's parameter names pick the constructor.
        selecting = viable_constructors(interface["arities"], len(ctor["arguments"])) > 1
        storage, arguments, problems, names = [], [], [], {}
        for p, item in zip(ctor["arguments"], named_values, strict=False):
            value = next(iter(item.values()))
            try:
                stored, argument = self._argument(
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
                    selecting,
                )
            except ValueError as error:
                problems.append(str(error))
                continue
            if stored is not None:
                storage.append(stored)
                names[stored[0]] = f"{identity}.args.{p['name']}"
            arguments.append(argument)
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
        checked = False
        if monitor:
            spelling = self.index.monitor_return(located)
            if spelling is None:
                checked = True
            elif spelling != "void":
                shape = type_shape(spelling)
                if shape[2] or shape[3] or is_arithmetic(spelling):
                    problems.append(
                        tr(
                            f"{identity}: OnMonitor of {module['name']} returns {spelling}; it "
                            "must return void",
                            f"{identity}: {module['name']} 的 OnMonitor 返回 {spelling}；"
                            "必须返回 void",
                        )
                    )
                else:
                    checked = True
        if problems:
            raise ValueError("\n".join(problems))
        label = module["id"] + ("<" + ", ".join(template_args) + ">" if template_args else "")
        return {
            "id": identity,
            "cpp_type": cpp_type,
            "label": label,
            "arguments": arguments,
            "index": i,
            "storage": storage,
            "storage_names": names,
            "monitor": monitor,
            "monitor_checked": checked,
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
        selecting: bool,
    ) -> tuple[tuple[str, str, str] | None, str]:
        """渲染一个构造参数。
        Render one constructor argument.

        依赖按名字绑定到注册名或排在前面的实例；值交给 ValueChecker。配置里新建的对象（花括号
        和映射）、引用参数和 std::initializer_list 的值放进紧邻实例之前的 static 变量，使其和
        实例活得一样久；标量、依赖名、nullptr 和 &名字 直接写在实参里。
        A dependency binds by name to a registration or an earlier instance; a value goes
        through ValueChecker. Objects the configuration creates (braces and mappings) and the
        values of reference and std::initializer_list parameters live in static variables right
        before the instance, as long as the instance; scalars, dependency names, nullptr and
        &name are written in the argument.

        Returns:
            (要先写出的 static 变量 (名字, 变量名之前的声明, 值) 或 None, 参数表达式)。
            (the static variable to write first as (name, declaration before the name, value)
            or None, the argument expression).

        Raises:
            ValueError: 名字无法绑定或值不符合规则。
                A name cannot be bound or a value breaks a rule.
        """
        field = f"{identity}.args.{p['name']}"
        typ = qualify(p["type"], interface, cpp_class, templates)
        target = qualify(p["type"], interface, cpp_class, templates, True)
        _, cv, pointers, reference = type_shape(target)
        if value is None and compile_check and p["default"] is None:
            raw = f"*static_cast<std::remove_reference_t<{typ}>*>(xr_ci_null)"
            return None, (f"static_cast<{typ}>({raw})" if typ.rstrip().endswith("&&") else raw)
        default = None
        if p["default"] is not None:
            default = initializer_tree(
                qualify(p["default"], interface, cpp_class, templates), target
            )
        if value is None and is_dependency(p):
            candidates = self._candidates(target, visible)
            raise ValueError(
                tr(
                    f"{field} is not filled in; candidates of type {target}: "
                    f"{', '.join(candidates) or 'none'}",
                    f"{field} 没有填写；类型为 {target} 的候选：{'、'.join(candidates) or '无'}",
                )
            )
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
                        return None, expression  # 同一类型直接绑定 / the same type binds directly
                    # 只有同名参数数的构造函数不止一个时才需要写出参数类型，选定要调用的那个。
                    # The parameter type is named only when several constructors could take
                    # the call.
                    return None, argument_text(expression, typ, False, selecting)
            expr, typed = self.checker.render(text, field, target, (), default)
        else:
            expr, typed = self.checker.render(value, field, target, (), default)
        braced = (
            isinstance(value, (dict, list)) or expr.lstrip().startswith("{") or typed
        ) and not pointers
        listed = "std::initializer_list<" in target.replace(" ", "")
        if not (reference or listed or braced):
            exact = typed or isinstance(value, (dict, list))
            return None, argument_text(expr, typ, exact, selecting)
        # 配置里的临时值和 initializer_list 的底层数组需要静态生存期。
        # Config temporaries and initializer-list backing arrays need static lifetime.
        storage = f"xr_{identity}_{p['name']}"
        moved = reference == "&&"
        if braced or listed:
            constant = (
                "" if reference == "&&" or (reference == "&" and "const" not in cv) else "const "
            )
            declaration = f"static {constant}{base_spelling(typ)}"
        else:
            # 名字和表达式按引用绑定：引用同一个对象，而不是复制它。
            # A name or expression binds by reference: it refers to the object instead of
            # copying it.
            declaration = f"static {typ}"
        return (storage, declaration, expr), (f"std::move({storage})" if moved else storage)

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
        constants: list[tuple[str, str, str]],
        namespace: str,
        source: str,
        location: tuple[str, str] | None,
        line_directives: bool,
        header_lines: tuple[str, ...] | list[str],
        compile_check: bool,
        selected: dict[str, dict],
    ) -> str:
        """拼出头文件：说明、include、常量、XRobotMain、主循环、两个宏和末尾的输入清单。
        Assemble the header: notice, includes, constants, XRobotMain, the monitor loop, the two
        macros and the input list at the end.

        XRobotMain 只接收实例用到的注册。每个实例前有一行注释说明它来自配置的哪一行；有
        #line 时，它在实例的语句之前指向配置，在实例之后指回头文件。
        XRobotMain takes only the registrations the instances use. A comment before each
        instance names the line of the configuration it comes from; with #line, one
        directive before the instance's statements points into the configuration and one
        after them points back into the header.
        """
        used = set()
        for entry in entries:
            for expression in (
                [entry["cpp_type"]]
                + entry["arguments"]
                + [f"{declaration} {name} {value}" for name, declaration, value in entry["storage"]]
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
        body: list = []  # 各实例的语句 / the statements of the instances
        back = object()  # 占位：#line 回到本头文件 / placeholder for "#line back into this header"
        directives = line_directives and location is not None
        for entry in entries:
            if body:
                body.append("")
            comment = f"{entry['id']}: {entry['label']}"
            if location is not None:
                comment += (
                    f" ({location[0]}:{entry['line']})" if entry["line"] else f" ({location[0]})"
                )
            body += wrap_comment(comment, INDENT)
            if directives and entry["line"]:
                body.append(f'#line {entry["line"]} "{_c_string(location[0])}"')
            for name, declaration, value in entry["storage"]:
                body += layout_declaration(declaration, name, value, ";", INDENT)
            if entry["arguments"]:
                body += layout_call(
                    f"static {entry['cpp_type']} {entry['id']}(",
                    entry["arguments"],
                    ");",
                    INDENT,
                )
            else:
                body.append(f"  static {entry['cpp_type']} {entry['id']};")
            if directives and entry["line"]:
                body.append(back)
        checks = [e["id"] for e in entries if e["id"] in monitored and e["monitor_checked"]]
        calls = [e["id"] for e in entries if e["id"] in monitored]
        if compile_check:
            lines = [
                "// Generated by `xrobot check-module` for Module CI; do not edit by hand.",
                "",
            ]
        else:
            lines = [header_banner(source if location is not None else None), "#pragma once", ""]
        constant_lines: list[str] = []
        if constants:
            constant_lines += [f"namespace {namespace}", "{"]
            for cpp_type, name, expr in constants:
                head = f"inline constexpr {cpp_type} {name}"
                head += "" if expr.lstrip().startswith("{") else " = "
                constant_lines += layout_value(head, expr, ";", 0)
            constant_lines += [f"}}  // namespace {namespace}", ""]
        # 用到的名字决定要包含的头文件。
        # The names used decide which headers are included.
        text = "\n".join(
            line for line in [*body, *constant_lines, *parameters] if isinstance(line, str)
        )
        implicit = "xrobot_generated::Implicit<" in text
        if implicit:
            text += " std::type_identity_t"
        if checks:
            text += " std::is_void_v"
        names = [f'"{m["name"]}.hpp"' for m in selected.values()]
        names += ['"libxr.hpp"', '"thread.hpp"']
        for header in config.get("constexpr_includes", []):
            header = header.strip()
            names.append(header if header.startswith("<") else f'"{header}"')
        names += _standard_headers(text)
        lines += sort_includes(names) + [""]
        if implicit:
            lines += [
                "namespace xrobot_generated",
                "{",
                "// Converts implicitly, so the compiler still warns about constants that change value.",
                "template <typename P>",
                "constexpr P Implicit(std::type_identity_t<P> value)",
                "{",
                "  return value;",
                "}",
                "}  // namespace xrobot_generated",
                "",
            ]
        lines += constant_lines
        if compile_check:
            lines += [
                "namespace xrobot_generated",
                "{",
                "void XRobotCompileCheck()",
                "{",
                "  // Compilation only: this function must never be invoked.",
                "  [[maybe_unused]] static void* xr_ci_null = static_cast<void*>(nullptr);",
            ]
            if body:
                lines.append("")
            lines += body
            lines += self._monitor_checks(checks)
            lines += [f"  {name}.OnMonitor();" for name in calls]
            lines += ["}", "}  // namespace xrobot_generated", ""]
            return self._numbered(lines, back, None)
        if local_templates:
            lines.append(f"template <{', '.join(local_templates)}>")
        lines += layout_call(
            "[[noreturn]] static inline void XRobotMain(",
            parameters,
            ")",
            0,
            hang_margin=3,
        )
        lines.append("{")
        lines += body
        if body:
            lines.append("")
        lines += self._monitor_checks(checks)
        lines += ["  for (;;)", "  {"]
        lines += [f"    {name}.OnMonitor();" for name in calls]
        sleep = config.get("settings", {}).get("monitor_sleep_ms", "1000")
        lines += [f"    LibXR::Thread::Sleep({sleep});", "  }", "}", ""]
        lines += [
            "// XR_REGISTER marks a name for the generator; XRobotMain binds and type-checks it.",
            "#define XR_REGISTER(name, ...) static_cast<void>(name)",
        ]
        call = "::XRobotMain" + (f"<{', '.join(actual_types)}>" if actual_types else "")
        lines += layout_macro("XROBOT_MAIN", call, [r["name"] for r in views])
        if header_lines:
            lines += ["", *header_lines]
        return self._numbered(lines, back, (location[1] if location else None))

    @staticmethod
    def _monitor_checks(names: list[str]) -> list[str]:
        """每个无法在生成时确认 OnMonitor 返回 void 的实例一条 static_assert。
        One static_assert for each instance whose OnMonitor is not known to return void at
        generation.
        """
        lines = []
        for name in names:
            lines += layout_call(
                "static_assert(",
                [
                    f"std::is_void_v<decltype({name}.OnMonitor())>",
                    f'"{name}.OnMonitor() must return void"',
                ],
                ");",
                INDENT,
            )
        return lines

    @staticmethod
    def _numbered(lines: list, back: object, header_name: str | None) -> str:
        """把占位换成指回头文件自身的 #line，并拼成文本。
        Replace the placeholders with #line directives that point back into the header
        itself, and join the text.
        """
        result, number = [], 1  # 下一行输出的行号 / line number of the next emitted line
        for line in lines:
            if line is back:
                line = f'#line {number + 1} "{_c_string(header_name)}"'
            result.append(line)
            number += line.count("\n") + 1
        return "\n".join(result) + ("" if result and result[-1] == "" else "\n")


def _c_string(path: str) -> str:
    """路径写在 #line 的字符串字面量中的样子。
    A path as it is written in the string literal of a #line.
    """
    return path.replace("\\", "\\\\").replace('"', '\\"')


_STANDARD_HEADERS = {
    "addressof": "<memory>",
    "initializer_list": "<initializer_list>",
    "move": "<utility>",
    "forward": "<utility>",
    "is_void_v": "<type_traits>",
    "add_lvalue_reference_t": "<type_traits>",
    "remove_reference_t": "<type_traits>",
    "type_identity_t": "<type_traits>",
}


def _standard_headers(text: str) -> list[str]:
    """生成的代码用到的标准库头文件。
    The standard library headers the generated code uses.
    """
    return sorted({h for n, h in _STANDARD_HEADERS.items() if re.search(rf"\bstd::{n}\b", text)})


def generate_code(
    project: Project,
    config_path: str | Path,
    modules: dict,
    registrations: list[dict],
    index: TypeIndex | None = None,
    line_directives: bool = True,
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
    entry = project.entry()
    depends = (
        ([project.lock] if project.lock.is_file() else [])
        + [entry]
        + sorted(set(module_headers(modules)))
    )
    generator = Generator(modules, index)
    return generator.render(
        config,
        registrations,
        source,
        config_path,
        project.header,
        project.header_lines(config_path, depends, entry),
        line_directives=line_directives,
    )


def load_modules(project: Project) -> dict:
    """BSP 中锁定的全部模块。
    Every Module locked in the BSP.
    """
    return discover_modules(project.modules_dir, project.lock)


def generate(
    project: Project,
    config_path: str | Path | None = None,
    modules: dict | None = None,
    index: TypeIndex | None = None,
    line_directives: bool = True,
) -> str:
    """为 config_path（缺省为当前选中的产品）生成 User/xrobot_main.hpp。
    Generate User/xrobot_main.hpp for config_path (default: the selected product).

    内容不变时不重写，修改时间保持不变。
    Unchanged content is not rewritten, and its modification time stays as it was.

    Args:
        modules, index: 调用方已读取的模块和类型索引（setup 检查全部配置时已建好）。
            Modules and type index the caller already has (setup builds them to check
            every configuration).
        line_directives: 每个实例前写 #line，使编译错误指回 YAML；False 时省略。
            Write a #line before each instance so compiler errors point back into the YAML;
            False omits them.

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
    code = generate_code(project, config_path, modules, registrations, index, line_directives)
    atomic_write(project.header, code)
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
