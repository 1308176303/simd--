import matplotlib.pyplot as plt
import matplotlib as mpl
import pandas as pd
import seaborn as sns
import numpy as np
import os
import matplotlib.font_manager as fm

# 方法一：使用已知存在的中文字体
plt.rcParams['font.sans-serif'] = ['Microsoft YaHei', 'SimHei', 'SimSun', 'Arial Unicode MS']
plt.rcParams['axes.unicode_minus'] = False
plt.rcParams['font.size'] = 12  # 设置合适的字体大小

# 创建字体对象 - 添加这行来解决font未定义的问题
font = fm.FontProperties(family=plt.rcParams['font.sans-serif'][0])

# 读取CSV数据
df = pd.read_csv('md5_performance_results.csv')


# 设置图表尺寸和风格
plt.figure(figsize=(15, 10))

# 1. 并行宽度对加速比的影响（总体对比）
plt.subplot(2, 2, 1)
df_avg = df.groupby('数据量')[['2路加速比', '4路加速比', '8路加速比']].mean().reset_index()
df_melted = df_avg.melt(id_vars=['数据量'], 
                        value_vars=['2路加速比', '4路加速比', '8路加速比'],
                        var_name='并行宽度', value_name='平均加速比')

sns.barplot(x='数据量', y='平均加速比', hue='并行宽度', data=df_melted)
plt.title('不同数据规模下各并行宽度的平均加速比', fontproperties=font)
plt.xlabel('数据量', fontproperties=font)
plt.ylabel('平均加速比', fontproperties=font)
plt.xticks(rotation=45)
plt.legend(title='并行宽度', prop=font)

# 2. 密码长度对加速比的影响（热力图）
plt.subplot(2, 2, 2)
pivot_8way = df.pivot(index='密码长度', columns='数据量', values='8路加速比')
sns.heatmap(pivot_8way, annot=True, fmt='.2f', cmap='YlGnBu')
plt.title('8路AVX2在不同数据量和密码长度下的加速比', fontproperties=font)
plt.xlabel('数据量', fontproperties=font)
plt.ylabel('密码长度', fontproperties=font)

# 3. 加速比与理论值比较
plt.subplot(2, 2, 3)
df_mean = df[['2路加速比', '4路加速比', '8路加速比']].mean()
theoretical = pd.Series([2, 4, 8], index=['2路加速比', '4路加速比', '8路加速比'])
comparison = pd.DataFrame({
    '实际加速比': df_mean,
    '理论加速比': theoretical
})

comparison.plot(kind='bar', ax=plt.gca())
plt.title('实际加速比与理论加速比的对比', fontproperties=font)
plt.xlabel('并行宽度', fontproperties=font)
plt.ylabel('加速比', fontproperties=font)
plt.grid(axis='y')
# 确保图例也使用中文字体
plt.legend(prop=font)

# 4. 密码长度对不同并行宽度的影响趋势
plt.subplot(2, 2, 4)
for size in df['数据量'].unique():
    df_size = df[df['数据量'] == size]
    plt.plot(df_size['密码长度'], df_size['8路加速比'], marker='o', label=f'数据量={size}')

plt.title('密码长度对8路AVX2加速比的影响', fontproperties=font)
plt.xlabel('密码长度', fontproperties=font)
plt.ylabel('8路AVX2加速比', fontproperties=font)
plt.grid(True)
plt.legend(prop=font)

plt.tight_layout()
plt.savefig('md5_performance_analysis.png', dpi=300)
plt.show()

# 创建额外的详细分析图
plt.figure(figsize=(15, 10))

# 5. 并行宽度与数据规模的交互效应
data_sizes = df['数据量'].unique()
width_types = ['2路加速比', '4路加速比', '8路加速比']
colors = ['#1f77b4', '#ff7f0e', '#2ca02c']

plt.subplot(2, 1, 1)
for i, width in enumerate(width_types):
    means = [df[df['数据量'] == size][width].mean() for size in data_sizes]
    plt.plot(data_sizes, means, marker='o', color=colors[i], label=width)

plt.title('数据规模对不同并行宽度加速比的影响', fontproperties=font)
plt.xlabel('数据量', fontproperties=font)
plt.ylabel('平均加速比', fontproperties=font)
plt.grid(True)
plt.legend(prop=font)

# 6. 并行宽度效率分析
plt.subplot(2, 1, 2)
# 计算效率：实际加速比/理论加速比
df['2路效率'] = df['2路加速比'] / 2 * 100
df['4路效率'] = df['4路加速比'] / 4 * 100
df['8路效率'] = df['8路加速比'] / 8 * 100

efficiency_data = df.groupby('数据量')[['2路效率', '4路效率', '8路效率']].mean().reset_index()
efficiency_melted = efficiency_data.melt(id_vars=['数据量'], 
                                       value_vars=['2路效率', '4路效率', '8路效率'],
                                       var_name='并行宽度', value_name='并行效率(%)')

sns.barplot(x='数据量', y='并行效率(%)', hue='并行宽度', data=efficiency_melted)
plt.title('不同数据规模下各并行宽度的平均并行效率', fontproperties=font)
plt.xlabel('数据量', fontproperties=font)
plt.ylabel('并行效率(%)', fontproperties=font)
plt.xticks(rotation=45)
plt.legend(title='并行宽度', prop=font)

plt.tight_layout()
plt.savefig('md5_efficiency_analysis.png', dpi=300)
plt.show()