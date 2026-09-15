# План: режимный слой clover2_fcu (mode_base, гейт setpoint, реестр режимов в bridge)

> **Статус: РЕАЛИЗОВАНО.** Дополнения к исходному плану, согласованные с
> владельцем в ходе выполнения:
> - **Одна фабрика-плагин**: bridge грузит только фабрику из параметра
>   `protocol` (lookup==protocol(), throw при ошибке). Механизм
>   per-capability overrides удалён: `CapabilityOverride.msg`, параметры
>   `overrides.*`, карта capability→фабрика в клиенте. Смешение источников
>   (напр. внешняя одометрия) — через провайдеров внутри пакета бекенда.
> - **Исключения в std::format-стиле** (`exception("...{}", args)`), no
>   error out-параметров.
> - **Клиент**: `get<T>()`/`get(name)` создают НОВЫЙ провайдер на каждый
>   вызов, ручка принадлежит вызывающему (кэша нет, px4-паритет).
> - **Тесты удалены из пакетов до окончания работ над ядром** (проблема
>   discovery в тестовом окружении не решена); писать заново после
>   стабилизации ядра — гейт (`stream_allowed`), режимный цикл
>   (register/lease/completed/bond-death), телеметрия/команды/параметры
>   через fake.
> - `to_string(result)` — метод `r.to_string()` (std::string_view).

Исполнитель: агент реализации. План самодостаточен, но архитектурные решения
пересматривать нельзя — они согласованы с владельцем проекта.

## 0. Контекст

- Workspace: `/home/motya/projects/coex/clover2-dev`, ROS 2 Jazzy.
- Сборка (всегда через `bash -c`, у пользователя zsh ломает `BASH_SOURCE`):
  ```
  cd /home/motya/projects/coex/clover2-dev && bash -c 'source /opt/ros/jazzy/setup.bash && colcon build --packages-select clover2_fcu_msgs clover2_fcu clover2_fcu_fake --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo'
  ```
- Тесты: `bash -c 'source install/setup.bash && ./build/clover2_fcu/test_clover2_fcu'`
  и `./build/clover2_fcu_fake/clover2_fcu_fake_tests`.
- Пакеты: `clover2_fcu_msgs` (msg/srv), `clover2_fcu` (ядро), `clover2_fcu_fake`
  (тестовый бекенд, pluginlib).
- Стиль: snake_case классы, `m_`-префикс полей, 4 пробела, C++20,
  ament_auto (образец — `clover2_fcu/CMakeLists.txt`), include-группы
  `// clover2 // ROS2 // third party // STL`. Исключения — только
  `clover2_fcu::exception` в стиле std::format: `exception("x {}", v)`.
- Не трогать: `clover2_nav/*`, `clover2_loc/*`, `clover2_common`, коммиты не делать.

Текущее состояние ядра (проверять по коду, не по памяти):

- Контракты: `include/clover2_fcu/capability/*_interface.hpp` + `all.hpp`
  (тип-лист `capability::all`, `all_names()`, концепт `capability::contract`,
  static_assert уникальности имён). Каждый контракт несёт
  `static constexpr const char* name` и `using shared_ptr`.
- Фабрика: `factory/factory_base.hpp` — pluginlib-база; `initialize(ctx, params)`
  → чистый виртуал `do_initialize`; регистрация `add<T>()` / `add<T>(builder)`
  (ключ — унаследованный `T::name`); `create(name|T, ctx, params)`;
  концепт `factory::is_capability` (контракт восстанавливается из
  `T::shared_ptr::element_type`).
- Клиент: `client.hpp` — наследник `context`; конструктор блокирующе получает
  конфиг у bridge (srv GetConfig); `get<T>()`/`get(name)` создают НОВЫЙ
  провайдер на каждый вызов (кэша нет, ручка принадлежит вызывающему);
  `m_capability_plugins`: все имена из `all_names()` → protocol, поверх —
  overrides; `has<T>()/has(name)`.
- Bridge: обычная нода (`clover2_common::node`), валидация в конструкторе
  (throw), параметры `protocol`, `overrides.<capability>`, `plugin_params`,
  сервис `get_config`; `pluginlib::ClassLoader` — value-член.
- `context`: node_context + type-erased расширения `provide<T>()/get<T>()`
  + `add_setpoint/setpoints` (осиротевшие — будут удалены в этой работе).
- Известная проблема окружения (не ваша задача): discovery сервиса bridge↔client
  в интеграционных тестах нестабилен; тесты, не требующие сервисного round-trip,
  обязаны быть зелёными.

## 1. Цели и зафиксированные решения

1. `mode_base` (имя согласовано, не controller_base) — единственный способ
   легально стримить setpoint'ы: регистрация в bridge, лиз «активен один режим»,
   таймер `update_setpoint(dt)` только при active.
2. Bridge — канонический реестр режимов на уровне ROS2 (работает с любым
   полётником, включая betaflight). Дублирование регистрации в сам FCU
   (`external_mode_interface`) — вне рамок этого этапа, только задел имён.
3. Живость режимов — bond (bondcpp), не свой heartbeat.
4. Создание setpoint-провайдеров через клиент остаётся открытым (как в px4),
   но публикация гейтится в ядре: провайдер без токена режима — muted.
   Это замена px4-механизма «FMU потребляет только активный режим».
5. Клиент остаётся транспортным уровнем (телеметрия/команды/параметры).

## 2. Этапы

### Этап A — clover2_fcu_msgs

Новые файлы:
- `msg/ModeState.msg`:
  ```
  # Активный режим (latched от bridge). mode_id == 0 — активного режима нет.
  uint32 mode_id
  string name
  ```
- `srv/RegisterMode.srv`: `string name` → `bool ok`, `uint32 mode_id`, `string error`.
- `srv/ActivateMode.srv`: `string name` → `bool ok`, `string error`.
  Пустое имя = деактивировать текущий (путь для mode_base::completed).

CMake уже гло́бирует `msg/*.msg` и `srv/*.srv` — новых зависимостей нет.
Чекпоинт: пакет собирается.

### Этап B — bridge: реестр режимов, лиз, bond

`clover2_fcu/bridge.hpp` / `src/bridge.cpp`:

- Члены:
  - `struct mode_record { std::string name; std::shared_ptr<bond::Bond> bond; };`
  - `std::map<uint32_t, mode_record> m_modes;`
  - `uint32_t m_next_mode_id{1}; uint32_t m_active_mode_id{0};`
  - `rclcpp::Publisher<clover2_fcu_msgs::msg::ModeState>::SharedPtr m_mode_state_pub`
    (QoS `transient_local`, depth 1; топик `mode_state`).
- Сервисы (создать в конструкторе, рядом с `get_config`):
  - `register_mode`: имя непустое и не дублирует существующее → выделить
    `mode_id`, создать `bond::Bond` на топике `<ns>/bond` с
    `id = std::to_string(mode_id)`, `on_broken` = режим умирает (см. ниже),
    запустить connect-поток по образцу nav2 lifecycle_manager
    (`bondcpp/bond.hpp`; смотреть, как bond создаётся в
    `nav2_lifecycle_manager`, — там ровно наша сторона «N клиентов»).
    Ответ `ok=true, mode_id`.
  - `activate_mode`: найти режим по имени (пустое имя — сброс), обновить
    `m_active_mode_id`, опубликовать `ModeState` (transient_local — поздние
    подписчики сразу получают текущее состояние).
- Обрыв bond: удалить запись режима; если он был активен — опубликовать
  `ModeState{0, ""}`.
- Зависимости: `bondcpp` добавить в `package.xml` (`<depend>`).

Чекпоинт: пакет собирается без предупреждений.

### Этап C — гейт setpoint в ядре

Файлы: `capability/setpoint_base.hpp`, четыре `setpoint_*_interface.hpp`,
`context.hpp`, `client.hpp`.

1. Новый тип в ядре (положить в `setpoint_base.hpp`):
   ```cpp
   /// Режимный токен: публикуется mode_base в контекст; setpoint-провайдер,
   /// созданный при наличии токена, привязан к этому режиму.
   struct mode_token { uint32_t mode_id; };
   ```
2. `context` — расширение `client_runtime` (положить рядом с mode_token или в
   `context.hpp`): `struct client_runtime { std::string bridge_node; };`
   Клиент публикует его в `client::apply_config`:
   `provide<client_runtime>(std::make_shared<client_runtime>(client_runtime{m_settings.bridge_node()}));`
3. `capability::setpoint_base` получает:
   - конструктор `explicit setpoint_base(context& ctx)`: читает через
     `ctx.get<mode_token>()` (отсутствие — легально, try/catch → «клиентский»
     провайдер) и `ctx.get<client_runtime>()` (bridge node; при отсутствии —
     дефолт `/fcu_bridge`);
   - подписку на `<bridge>/mode_state` (transient_local), кэш активного id;
   - `protected: bool stream_allowed() const noexcept;` — true только если
     токен есть и `active_mode_id == token.mode_id`. При отказе —
     `RCLCPP_WARN_THROTTLE` (1 раз в 5 с, текст «setpoint provider is muted:
     no active mode / mode not active; create setpoints via mode_base»);
   - удалить осиротевшую активацию: `active()/set_active/on_activate/
     on_deactivate/friend client`, а также `context::add_setpoint/setpoints`
     и тест `context_registers_setpoints` в core.cpp.
4. Контракты `setpoint_{position,velocity,attitude,rates}_interface`:
   добавить конструкторы `explicit X(context& ctx) : setpoint_base(ctx) {}`.
5. Контракт для бекендов (задокументировать в заголовке контракта):
   **провайдер обязан проверять `stream_allowed()` перед каждым publish**
   и молча пропускать публикацию при false (mute, не throw).

Чекпоинт — тест в `clover2_fcu/test/core.cpp` (без discovery):
- минимальный тестовый провайдер от `setpoint_position_interface` прямо в
  тесте (update() проверяет stream_allowed и пишет флаг);
- кейс 1: контекст без mode_token → всегда muted;
- кейс 2: provide<mode_token>{id}, тестовая нода публикует ModeState{id} →
  stream разрешён; ModeState{0} → снова muted.

### Этап D — mode_base

Новые `include/clover2_fcu/mode_base.hpp`, `src/mode_base.cpp`.
Зависимости пакета: + `bondcpp`, + `clover2_fcu_msgs` (уже есть).

```cpp
namespace clover2_fcu {

class mode_base {
public:
    struct settings {
        std::string name;               // непустое, уникальное у bridge
        float update_rate_hz{30.f};
        // fluent-сеттеры по образцу client::settings
    };

    mode_base(client& fcu, const settings& s);
    virtual ~mode_base();               // ломает bond; bridge сам деактивирует

    bool active() const;
    void completed(result r);           // ActivateMode("") + лог результата

protected:
    virtual void on_activate() {}
    virtual void on_deactivate() {}
    virtual void update_setpoint(double dt_s) { (void)dt_s; }

    client& fcu();

    // Единственный легальный путь к setpoint'ам: ручки создаются лениво,
    // кэшируются в режиме, токен уже в контексте (см. ниже).
    std::shared_ptr<capability::setpoint_position_interface> position();
    std::shared_ptr<capability::setpoint_velocity_interface> velocity();
    std::shared_ptr<capability::setpoint_attitude_interface> attitude();
    std::shared_ptr<capability::setpoint_rates_interface> rates();

private:
    void handle_mode_state(const clover2_fcu_msgs::msg::ModeState& msg);

    client& m_fcu;
    settings m_settings;
    uint32_t m_mode_id{0};
    bool m_active{false};
    std::unique_ptr<bond::Bond> m_bond;
    rclcpp::Subscription<clover2_fcu_msgs::msg::ModeState>::SharedPtr m_mode_state_sub;
    rclcpp::TimerBase::SharedPtr m_update_timer;
    rclcpp::Time m_last_update;
    std::shared_ptr<capability::setpoint_position_interface> m_position;  // + 3 остальных
};
}
```

Механика конструктора:
1. Вызвать `RegisterMode` синхронно. Паттерн — как `client::connect`
   (callback group + ожидание future); общую часть вынести в
   `utils/sync_service.hpp`:
   `response_t call_sync(group, client, request, timeout)` — кинуть
   `exception` при таймауте. Использовать и в client::connect.
2. Создать клиентскую сторону bond на `<bridge>/bond`,
   `id = std::to_string(mode_id)`; `on_broken` → локальная деактивация
   (гейт и так замолчит, но остановить таймер и позвать on_deactivate).
3. `m_fcu.provide<mode_token>(std::make_shared<mode_token>(mode_token{m_mode_id}));`
4. Подписка на `<bridge>/mode_state` (transient_local).
5. `handle_mode_state`: `msg.mode_id == m_mode_id` → (если не был)
   `on_activate()` + запуск таймера с периодом `1/update_rate_hz`
   (таймер зовёт `update_setpoint(dt)` с dt по часам контекста);
   иначе → стоп таймера + `on_deactivate()`.

Документировать в заголовке: нода-хозяин должна крутиться в момент
конструирования mode_base (то же требование, что у client).

### Этап E — fake-бекенд и тесты

1. Актуализировать `clover2_fcu_fake` под текущий API клиента (сейчас тесты
   не компилируются): ручки `auto odom = m_client->odometry(); odom->last()`;
   убрать `connected()/protocol()/activate()/deactivate()` и `bridge->configure()`
   (bridge — обычная нода).
2. Fake setpoint-провайдеры: конструкторы контрактов с `context&` (передать
   в базу), в `update()` проверять `stream_allowed()`; в реестре заменить
   `was_active` на `was_streaming` (записывать результат гейта).
3. Новые кейсы в `test/end_to_end.cpp`:
   - «setpoint без режима»: `m_client->get<setpoint_position_interface>()`,
     `update()` → записей с was_streaming=true нет, исключений нет;
   - «режимный цикл»: mode_base на клиентской ноде; активация прямым
     сервисным вызовом ActivateMode к bridge → в update_setpoint стримит
     (was_streaming=true); ActivateMode("") → замолчал;
   - «смерть режима»: уничтожить mode_base (разрыв bond) → bridge публикует
     ModeState{0}.
   Эти кейсы зависят от discovery bridge↔client — пометить и не блокировать
   приёмку, если падают по известной проблеме окружения (см. §0).
4. `test/core.cpp`: тесты гейта из Этапа C; удалить осиротевший тест
   setpoint-реестра.

### Этап F — приёмка

1. `colcon build --packages-select clover2_fcu_msgs clover2_fcu clover2_fcu_fake`
   — без ошибок и без предупреждений.
2. `test_clover2_fcu` — все зелёные.
3. `clover2_fcu_fake_tests` — зелёные все, кроме помеченных discovery-зависимых;
   список упавших с причиной — в отчёт.
4. Отчёт: что сделано, список изменённых/новых файлов, известные падения,
   открытые вопросы (если появились).

## 3. Вне рамок (не делать)

- `external_mode_interface` — дублирование регистрации режимов в FCU (px4).
- Watchdog «FCU выпал из offboard» в mode_base (следующий этап).
- Mission/executor-слой, README-документация (кроме doxygen в заголовках).
- Правки discovery-проблемы окружения.

## 4. Риски и подсказки

- bondcpp: правильная инициализация сторон смотреть в nav2
  (`nav2_lifecycle_manager` — сторона многих bond'ов, как наш bridge;
  `nav2_lifecycle`/`bondcpp` examples — сторона клиента).
- `ModeState` обязан быть `transient_local` — поздние mode_base обязаны
  узнать текущее состояние сразу.
- Однопоточный executor — базовое допущение всей библиотеки; в колбеках
  гейта и mode_base синхронизация не нужна.
- Проверять сборку после каждого этапа, не копить ошибки.
