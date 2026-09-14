# Настройка питания

Чтобы настроить параметры питания, откройте в QGroundControl вкладку *Vehicle Setup* и выберите меню *Power*.

## Калибровка делителя напряжения

```{note}
Калибровку делителя напряжения нужно выполнять с подключенным аккумулятором.
```

1. В QGroundControl перейдите в *Vehicle Configuration* и выберите меню *Power*.
2. Установите параметр *Number of cells* = **6S**.
3. Подключите индикатор напряжения к балансировочному разъему аккумуляторной батареи.
4. Нажмите *Calculate* напротив параметра *Voltage Divider*.
5. Введите полученное значение в открывшееся поле.
6.    Нажмите *Close*, чтобы сохранить рассчитанное значение.

Если индикатора напряжения нет или ручная калибровка невозможна, используйте усредненное значение `Voltage divider = 21`.

```{figure} ../../assets/common/setup/qgc-voltage-divider.webp
:alt: Калибровка делителя напряжения в QGroundControl
:width: 90%
:align: center

Калибровка делителя напряжения
```
<br>

Дополнительная информация: [QGroundControl Power Setup](https://docs.qgroundcontrol.com/master/en/qgc-user-guide/setup_view/power.html).
