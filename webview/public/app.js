document.addEventListener('DOMContentLoaded', () => {
    const editor = CodeMirror.fromTextArea(document.getElementById('code-editor'), {
        lineNumbers: true,
        mode: 'pascal',
        theme: 'material-darker',
        lineWrapping: true,
    });

    // Provide a default example code
    const exampleCode = `

    //测试用例

    program TestProgram
        var i, sum, product, j, k : integer;
    
        procedure aaaa(u:integer);
            var a,b :integer;
            begin
                a:=u;
                b:=a
            end;
    
        function qqqq(v:integer):integer;
            var c,d :integer;
            begin
                qqqq:=v
            end;
    begin
        i := 0.5+0.5;
        j := 1;
        k := j;
        sum := 0;
        product := 1;
    
        while i <= 10 do  //循环语句
        begin
            sum := sum + i;
            product := product * i;
            i := i + 1
        end;
          k := k+1;	
         i:=k;
        sum:=qqqq(i);
    
        if sum > 50 then  //条件语句
            product := 0
        else
            sum := 0
    end.`;
    editor.setValue(exampleCode);

    const compileBtn = document.getElementById('compile-btn');
    const outputDisplay = document.getElementById('output-display');
    const errorDisplay = document.getElementById('error-display');
    const loadingSpinner = document.getElementById('loading-spinner');

    compileBtn.addEventListener('click', async () => {
        const code = editor.getValue();
        const target = document.querySelector('input[name="target"]:checked').value;

        outputDisplay.textContent = '';
        errorDisplay.textContent = '';
        errorDisplay.style.display = 'none';
        loadingSpinner.style.display = 'block';

        try {
            const response = await fetch('http://localhost:3000/api/compile', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ code, target }),
            });

            const result = await response.json();

            if (!response.ok) {
                // Handle compiler errors or server-side issues
                let errorMessage = `Error: ${result.error || 'Unknown error'}`;
                if (result.details) {
                    errorMessage += `\n\nDetails:\n${result.details}`;
                }
                if(result.partial_output) {
                    errorMessage += `\n\nPartial Output:\n${result.partial_output}`;
                }
                showError(errorMessage);
            } else {
                // Render successful output
                renderOutput(result, target);
            }
        } catch (error) {
            console.error('Fetch error:', error);
            showError(`Network or server error: ${error.message}`);
        } finally {
            loadingSpinner.style.display = 'none';
        }
    });

    function renderOutput(data, target) {
        // Clear previous output
        outputDisplay.innerHTML = ''; // Use innerHTML to render HTML tags

        switch (target) {
            case 'tokens':
                renderTokens(data);
                break;
            case 'quads':
                renderQuads(data);
                break;
            case 'symbols':
                renderSymbols(data);
                break;
            default:
                outputDisplay.textContent = JSON.stringify(data, null, 2);
        }
    }

    function renderTokens(data) {
        // Render the token sequence string
        const sequenceHeader = document.createElement('h3');
        sequenceHeader.textContent = 'Token 序列';
        const sequenceBlock = document.createElement('div');
        sequenceBlock.className = 'code-block';
        sequenceBlock.textContent = data.sequence || '无 Token 序列';
        outputDisplay.appendChild(sequenceHeader);
        outputDisplay.appendChild(sequenceBlock);

        // Render the tables
        if (data.tables) {
            const tablesHeader = document.createElement('h3');
            tablesHeader.textContent = '符号表系统';
            outputDisplay.appendChild(tablesHeader);
            
            const tableContainer = document.createElement('div');
            tableContainer.className = 'table-grid';
            
            tableContainer.appendChild(createTable(data.tables.keywords, ['编号', '关键字'], ['index', 'value']));
            tableContainer.appendChild(createTable(data.tables.operators, ['编号', '界符'], ['index', 'value']));
            tableContainer.appendChild(createTable(data.tables.identifiers, ['编号', '标识符'], ['index', 'value']));
            tableContainer.appendChild(createTable(data.tables.constants, ['编号', '常量'], ['index', 'value']));
            
            outputDisplay.appendChild(tableContainer);
        }
    }

    function renderQuads(data) {
        if (!data || !data.before || !data.after) {
             outputDisplay.textContent = '无法加载四元式数据。';
             return;
        }

        // Render "before" quads
        const beforeHeader = document.createElement('h3');
        beforeHeader.textContent = '优化前四元式';
        const beforeBlock = document.createElement('div');
        beforeBlock.className = 'code-block';
        if (Array.isArray(data.before) && data.before.length > 0) {
            beforeBlock.innerHTML = data.before.map(q => 
                `${q.line}: (${q.op}, ${q.arg1}, ${q.arg2}, ${q.result})`
            ).join('<br>');
        } else {
            beforeBlock.textContent = '无四元式生成';
        }
        
        outputDisplay.appendChild(beforeHeader);
        outputDisplay.appendChild(beforeBlock);

        // Render "after" quads
        const afterHeader = document.createElement('h3');
        afterHeader.textContent = '优化后四元式';
        const afterBlock = document.createElement('div');
        afterBlock.className = 'code-block';
        if (Array.isArray(data.after) && data.after.length > 0) {
            afterBlock.innerHTML = data.after.map(q => 
                `${q.line}: (${q.op}, ${q.arg1}, ${q.arg2}, ${q.result})`
            ).join('<br>');
        } else {
            afterBlock.textContent = '无优化后四元式';
        }

        outputDisplay.appendChild(afterHeader);
        outputDisplay.appendChild(afterBlock);
    }

    function renderSymbols(data) {
        if (!data) return;

        const container = document.createElement('div');
        container.className = 'table-grid';

        // Main Symbol Table (now part of the grid)
        const symbolHeaders = ['名称', '类型', '类别', '地址'];
        const symbolKeys = ['name', 'type', 'category', 'address'];
        container.appendChild(createTable(data.symbols, symbolHeaders, symbolKeys, '符号表'));
        
        // Constants Table
        const constantHeaders = ['编号', '值'];
        const constantKeys = ['index', 'value'];
        container.appendChild(createTable(data.constants, constantHeaders, constantKeys, '常数表'));
        
        // Activation Record
        const activationHeaders = ['地址', '名称'];
        const activationKeys = ['address', 'name'];
        container.appendChild(createTable(data.activation_record, activationHeaders, activationKeys, '活动记录映像'));
        
        outputDisplay.appendChild(container);
    }

    function createTable(data, headers, keys, title) {
        const container = document.createElement('div');
        container.className = 'table-wrapper';

        if (title) {
            const tableTitle = document.createElement('h4');
            tableTitle.textContent = title;
            container.appendChild(tableTitle);
        }

        const table = document.createElement('table');
        const thead = table.createTHead();
        const tbody = table.createTBody();
        
        const headerRow = thead.insertRow();
        headers.forEach(headerText => {
            const th = document.createElement('th');
            th.textContent = headerText;
            headerRow.appendChild(th);
        });

        if (Array.isArray(data)) {
            data.forEach(rowData => {
                const row = tbody.insertRow();
                keys.forEach(key => {
                    const cell = row.insertCell();
                    cell.textContent = rowData[key] !== undefined ? rowData[key] : '-';
                });
            });
        }
        
        container.appendChild(table);
        return container;
    }

    function showError(message) {
        errorDisplay.textContent = message;
        errorDisplay.style.display = 'block';
    }
}); 