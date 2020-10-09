% Copyright 2020 Junekey Jeon
%
% The contents of this file may be used under the terms of
% the Apache License v2.0 with LLVM Exceptions.
%
%    (See accompanying file LICENSE-Apache or copy at
%     https://llvm.org/foundation/relicensing/LICENSE.txt)
%
% Alternatively, the contents of this file may be used under the terms of
% the Boost Software License, Version 1.0.
%    (See accompanying file LICENSE-Boost or copy at
%     https://www.boost.org/LICENSE_1_0.txt)
%
% Unless required by applicable law or agreed to in writing, this software
% is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
% KIND, either express or implied.

function [avg] = plot_fixed_precision_benchmark( filename )

output_filename = replace(filename, '.csv', '.pdf');

table = readtable(filename);
max_precision = table{size(table,1),2};
number_of_algorithms = size(table,1) / (max_precision + 1);
names = string(zeros(number_of_algorithms, 1));
for i=1:number_of_algorithms
    names(i) = string(table{i * (max_precision + 1),1});
end

% algorithm index, precision, measured_time
data = zeros(number_of_algorithms, max_precision + 1);
for algorithm_idx=1:number_of_algorithms
    for precision=0:max_precision
        data(algorithm_idx, precision + 1) = ...
            table{(algorithm_idx - 1) * (max_precision + 1) + precision + 1, 3};
    end
end

color_array = {[.8 .06 .1],[.1 .7 .06],[.06 .1 .8],[.6 .2 .8],[.8 .9 0],[.5 .6 .7],[.8 .2 .6]};

% plot
fig = figure('Color','w','DefaultLegendInterpreter','none');
fig_handles = zeros(number_of_algorithms,1);
hold on
for algorithm_idx=1:number_of_algorithms
    fig_handles(algorithm_idx) = plot(1:(max_precision+1),data(algorithm_idx,:), ...
        'Color', color_array{algorithm_idx}, 'LineWidth', 1.2);
end
legend(fig_handles,names);
axis([0 max_precision -Inf Inf]);
xlabel('Precision + 1');
ylabel('Time (ns)');
h = gca;
h.XTick = [1 2 3 4 5 6 7 8 9 10 20 30 40 50 60 70 80 90 100 200 300 400 500 600 700 800];
h.XGrid = 'on';
h.YGrid = 'on';
set(gca,'xscale','log');
set(gca,'yscale','log');
set(gca,'TickLabelInterpreter', 'latex');
set(gcf, 'Position', [100 100 1200 500]);
orient(fig,'landscape');
print(fig, output_filename,'-dpdf');
hold off

end
