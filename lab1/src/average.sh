if [ $# -eq 0 ]; then
    echo "Ошибка: Не указаны аргументы"
    echo "Использование: ./average.sh число1 число2 ..."
    exit 1
fi

sum=0
count=0

for num in "$@"; do
    if [[ ! $num =~ ^-?[0-9]+$ ]]; then
        echo "Ошибка: '$num' не является целым числом" >&2
        exit 1
    fi
    sum=$((sum + num))
    count=$((count + 1))
done

average=$((sum / count))

echo "Количество чисел: $count"
echo "Сумма чисел: $sum"
echo "Среднее арифметическое: $average"
