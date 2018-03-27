
    %%   
    % MATLAB script for the visualization of the results of OpenCV-Features-Comparison  
    % Copyright (c) by Yuhua Zou.   
    % Email: yuhuazou AT gmail DOT com OR chenyusiyuan AT 126 DOT com  
    %  
      
    close all;  
    clear all;  
    clc;  
      
    % workroot: directory which contains files as follows:  
    %     HomographyError.txt  
    %     MatchingRatio.txt  
    %     MeanDistance.txt  
    %     PercentOfCorrectMatches.txt  
    %     PercentOfMatches.txt  
    %     Performance.txt  
    %  
    %workroot='E:\\mcode\FEAT_compare\';  
    workroot='E:\\Local Repositories\NiuKe\opencv3_FeatCompare\'; 
    files=dir([workroot,'***.txt']);  %�г�����txt�ļ�
      
    % use the file name as the figure name, stored in a cell 'nameFigure'  
    nameFigure = cell(1,length(files));%  cell(M,N) is an M-by-N cell array of empty matrices. 
      
    for i=1:length(files),  
        % get file name and create a correspoinding figure  
        filename = files(i,1).name; %���ʵ�i���ļ��� 
        nameFigure{i} = filename(1:end-4);  
        figure('Name',nameFigure{i},'Position',[20 40 1240 780]);   
          
        % initialize 2 cells to store title name and legends of each plot  
        nameTitle{1} = '';  
        nameLegend{1} = '';     
          
        % open file  
        file = fullfile(workroot,filename);  
        fid = fopen(file,'r');  %ֻ����ʽ��
          
        % process 'Performance.txt' individually   'Performance.txtҪ��������
        if strcmp(nameFigure{i},'Performance') , %�ַ����Ƚϣ������performance.txt 
            nl = 0;  %��ʼ����0
            data = 0;  
              
            %% analyze each line  
            tline = fgetl(fid);  %��ȡÿһ������
            while ischar(tline),  % �ж������Ƿ����ַ��͵�
                nl = nl + 1;          
                tline(tline == '"') = '';  %ȥ��˫���ţ�
                if nl == 1,  
                    nameTitle{ 1 } = tline; %�ַ��͵ľ���title������һ��cell 
                elseif nl == 2,  %txt�ڶ���
                    args = regexp(tline,'\t','split');%�ָ��ַ���  
                    nameLegend = args(2:end);  %ͼ��"Average time per Frame"	"Average time per KeyPoint"
                elseif ~isempty(tline), %�����ݵĵ�����֮�����
                    args = regexp(tline,'\t','split');  
                    cols = length(args) - 1;  %���ݵ�����
                    tick = args{1};   
                    nameTick{nl-2} = tick;  %����㷨����
                    for n = 1:cols, data(nl-2,n) = str2num( args{n+1} ); end  
                    %data������� ��һ��5x2������
                end  
                tline = fgetl(fid);  
            end  
              
            % plotting  
            for k=1:3,  
                subplot(3,1,k);  
                [data_sorted,idx] = sort(data(:,k),'ascend');  %��������
                h = barh( data_sorted,'group' ); % get the handle to change bar color��ˮƽֱ��ͼ������һ�����              
                xlabel('Time (ms)'); ylabel('Algorithms');  
                title(nameLegend{ k }, 'FontWeight', 'bold'); %�������������ϸ��Bold���� 
                set(gca, 'yticklabel', nameTick(idx), 'FontSize', 7);  %�����С����pointsΪ��λ
                %set��gca����ȡ���������ԣ������
    %             set(gca,'yticklabel','','FontSize',7); % unshow y-axis ticks  
      
                %% attach the value to the right side of each bar  
                x = get(h, 'XData'); %x=1 2 3 4 5  
                y = get(h, 'YData');  %y=������ֵ
                horiGap = 0.01 * ( max(y) - min(y) );  
                for c=1:length(x),  
                    text( y(c) + horiGap, x(c), num2str(y(c), '%0.3f'),...  
                        'HorizontalAlignment','left','VerticalAlignment','middle',...  
                        'FontSize',7);                  
                end  
                  
                %% Change the color of each bar  
                ch = get(h,'Children'); % get children of the bar groupֱ��ͼĬ����Ⱥ�������õ���bar
                %�ϵ㿴��ch�ǿյģ�˵��Barû���Ӷ���
                fvd = get(ch,'Faces'); % get faces data  matlab��ͼ�߼�����
                fvcd = get(ch,'FaceVertexCData'); % get face vertex cdata  
                 [~, izs] = sortrows(data_sorted,1); % sort the rows ascending by first columnsָ������Ƚϵ���
                % ���ͼ������������ֵ�Ĵ�С��ɫ��ÿ���е�ֵԽ����ɫԽͻ�� 
                
%                for c = 1:length(data_sorted)
%                    row=izs(c);
%                     fvcd(fvd(row,:)) = c%idx(c,1); % adjust the face vertex cdata to be that of the row 
%                     %idx������֮��� 5x1����
%                end  
%                 set(ch,'FaceVertexCData',fvcd) % set to new face vertex cdata  
                %set(ch,'FaceVertexCData',[0 0 1;0 1 1;1 1 1;1 0 1;1 0 1])
                %h(1).FaceColor='red';%���������bar���ɺ�ɫ��
                set(get(h(1),'BaseLine'),'LineWidth',2,'LineStyle',':')
                %colormap summer % Change the color scheme
                k = 128; % ׼������128 *3 �е�colormap
colormap(summer(k)); % ���������һ��128 * 3�ľ��󣬷ֱ����[R G B]��ֵ
                
                %set(h,'LineWidth',1,'EdgeColor','red');
                % you can search 'FaceVertexCData' in MATLAB Help for more info.  
                %�ʼ�ڸı�ˮƽ����ɫ��������ˣ�����forѭ��û�ܻ��Ƶڶ���ͼ
            end  
        else  
        %% process other documents  
            nDataRow = 0;   % rows of numerical data in each plot  
            nPlot = 0;      % number of plots  
            data{1} = 0;    % all numerical data in current document  
              
            %% analyze each line  
            tline = fgetl(fid);  
            while ischar(tline) && ~strcmp(tline, -1),    
                % split the line into strings by '\t'      
                args = regexp(tline,'\t','split');  
                if strcmp(args{end},''), args = args(1:end-1); end; % remove the last empty one  
                  
                % the line which contains only one string   
                % is recognized as the beginning of a new plot  
                % the string is stored as plot title  
                % which represents the transformation type  
                if length(args) == 1,  
                    nDataRow = 0;  
                    nPlot = nPlot + 1;  
                    tline(tline == '"') = '';  
                    nameTitle{ nPlot } = tline;  
                else  
                    % the line with several '"'s under the 'plot title' line  
                    % stores legends of the plot  
                    % which represent feature methods  
                    if ~isempty( find( tline=='"', 1 ) ),  
                        tline(tline == '"') = '';   
                        nameLegend{ nPlot } = args(2:end);  
                    else  
                    % the line without '""'s contains numerical data  
                    % which represent experiment data  
                        nDataRow = nDataRow + 1;  
                        for n = 1:length(args),   
                            data{ nPlot }(nDataRow,n) = str2double( args{n} );   
                        end  
                    end  
                end  
                tline = fgetl(fid);  
            end            
              
            %% plotting  
            cmap = colormap( jet( length( nameLegend{1} ) ) ); % cmap: table of line color  
            for p = 1:nPlot,  
                subplot(ceil(nPlot/2), 2, p);   
                xdata = data{p}(:,1);  
                ydata = data{p}(:,2:end);  
                for r=1:size(ydata,2)  
                    plot(xdata, ydata(:,r), 'Color', cmap(r,:), 'LineWidth',2); hold on; % draw each line with different color  
                end  
                title(nameTitle{p},'FontWeight','bold');  
                if p == 1, legend(nameLegend{p},'Location','Best','FontSize',7); end  
                xlim([min(xdata(:)-0.1*max(xdata(:))), 1.1*max(xdata(:))]);  
                ylim([0, 1.1*max(ydata(:))]);  
            end  
        end     
          
        fclose(fid);  
    end  