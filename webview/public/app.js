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

    const compileBtnTokens = document.getElementById('compile-btn-tokens');
    const compileBtnQuads = document.getElementById('compile-btn-quads');
    const compileBtnSymbols = document.getElementById('compile-btn-symbols');
    const compileBtnTargetCode = document.getElementById('compile-btn-target-code');

    const outputDisplay = document.getElementById('output-display');
    const errorDisplay = document.getElementById('error-display');
    const loadingSpinner = document.getElementById('loading-spinner');
    
    const tokenTableContainer = document.getElementById('token-table-container');
    const quadTableContainer = document.getElementById('quad-table-container');
    const symbolContainer = document.getElementById('symbol-table-container');
    const targetCodeContainer = document.getElementById('target-code-container');

    const exampleSelect = document.getElementById('example-select');

    compileBtnTokens.addEventListener('click', () => fetchAndCompile(editor.getValue(), 'tokens'));
    compileBtnQuads.addEventListener('click', () => fetchAndCompile(editor.getValue(), 'quads'));
    compileBtnSymbols.addEventListener('click', () => fetchAndCompile(editor.getValue(), 'symbols'));
    compileBtnTargetCode.addEventListener('click', () => fetchAndCompile(editor.getValue(), 'target_code'));

    function hideAllContainers() {
        tokenTableContainer.style.display = 'none';
        quadTableContainer.style.display = 'none';
        symbolContainer.style.display = 'none';
        targetCodeContainer.style.display = 'none';
        outputDisplay.style.display = 'none';
    }

    async function fetchAndCompile(sourceCode, target) {
        outputDisplay.textContent = '';
        outputDisplay.style.display = 'none';
        errorDisplay.textContent = '';
        errorDisplay.style.display = 'none';
        loadingSpinner.style.display = 'block';
        hideAllContainers();

        try {
            const response = await fetch('http://localhost:3000/api/compile', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ code: sourceCode, target }),
            });

            const result = await response.json();

            if (!response.ok) {
                let errorMessage = `Error: ${result.error || 'Unknown error'}`;
                if (result.details) {
                    errorMessage += `\n\nDetails:\n${result.details}`;
                }
                if(result.partial_output) {
                    errorMessage += `\n\nPartial Output:\n${result.partial_output}`;
                }
                showError(errorMessage);
            } else {
                if (target === 'tokens') {
                    tokenTableContainer.style.display = 'block';
                    renderTokenTable(result);
                } else if (target === 'quads') {
                    quadTableContainer.style.display = 'block';
                    renderQuadTables(result);
                } else if (target === 'symbols') {
                    symbolContainer.style.display = 'block';
                    renderSymbolTable(result);
                } else if (target === 'target_code') {
                    targetCodeContainer.style.display = 'block';
                    renderTargetCodeTable(result);
                } else {
                    outputDisplay.style.display = 'block';
                    outputDisplay.textContent = JSON.stringify(result, null, 2);
                }
            }
        } catch (error) {
            console.error('Fetch error:', error);
            showError(`Network or server error: ${error.message}`);
        } finally {
            loadingSpinner.style.display = 'none';
        }
    }

    function renderTokenTable(data) {
        tokenTableContainer.innerHTML = ''; // Clear previous content

        // Render the token sequence string
        const sequenceHeader = document.createElement('h3');
        sequenceHeader.textContent = 'Token 序列';
        const sequenceBlock = document.createElement('div');
        sequenceBlock.className = 'code-block';
        sequenceBlock.textContent = data.sequence || '无 Token 序列';
        tokenTableContainer.appendChild(sequenceHeader);
        tokenTableContainer.appendChild(sequenceBlock);

        // Render the tables
        if (data.tables) {
            const tablesHeader = document.createElement('h3');
            tablesHeader.textContent = '符号表系统';
            tokenTableContainer.appendChild(tablesHeader);
            
            const tableGrid = document.createElement('div');
            tableGrid.className = 'table-grid';
            
            tableGrid.appendChild(createTable(data.tables.keywords, ['编号', '关键字'], ['index', 'value']));
            tableGrid.appendChild(createTable(data.tables.operators, ['编号', '界符'], ['index', 'value']));
            tableGrid.appendChild(createTable(data.tables.identifiers, ['编号', '标识符'], ['index', 'value']));
            tableGrid.appendChild(createTable(data.tables.constants, ['编号', '常量'], ['index', 'value']));
            
            tokenTableContainer.appendChild(tableGrid);
        }
    }

    function renderQuadTables(data) {
        if (!data || !data.before || !data.after) {
             // Find a more appropriate way to show this error, maybe inside the container
             const container = document.getElementById('quad-table-container');
             container.innerHTML = '<h3>无法加载四元式数据。</h3>';
             return;
        }

        const beforeTbody = document.querySelector("#quad-table-before tbody");
        const afterTbody = document.querySelector("#quad-table-after tbody");

        beforeTbody.innerHTML = "";
        afterTbody.innerHTML = "";

        // Render "before" quads
        if (Array.isArray(data.before)) {
            data.before.forEach(q => {
                const row = beforeTbody.insertRow();
                // Handle separators
                if(q.line === "") {
                    const cell = row.insertCell();
                    cell.colSpan = 5;
                    cell.className = 'separator';
                    cell.textContent = q.op;
                } else {
                    row.insertCell().textContent = q.line;
                    row.insertCell().textContent = q.op;
                    row.insertCell().textContent = q.arg1;
                    row.insertCell().textContent = q.arg2;
                    row.insertCell().textContent = q.result;
                }
            });
        }

        // Render "after" quads
        if (Array.isArray(data.after)) {
            data.after.forEach(q => {
                const row = afterTbody.insertRow();
                 // Handle separators
                 if(q.line === "") {
                    const cell = row.insertCell();
                    cell.colSpan = 5;
                    cell.className = 'separator';
                    cell.textContent = q.op;
                } else {
                    row.insertCell().textContent = q.line;
                    row.insertCell().textContent = q.op;
                    row.insertCell().textContent = q.arg1;
                    row.insertCell().textContent = q.arg2;
                    row.insertCell().textContent = q.result;
                }
            });
        }
    }

    function renderSymbolTable(data) {
        symbolContainer.innerHTML = ''; // Clear previous content
        if (!data) return;

        const tableGrid = document.createElement('div');
        tableGrid.className = 'table-grid';

        // Main Symbol Table (now part of the grid)
        const symbolHeaders = ['名称', '类型', '类别', '地址'];
        const symbolKeys = ['name', 'type', 'category', 'address'];
        tableGrid.appendChild(createTable(data.symbols, symbolHeaders, symbolKeys, '符号表'));
        
        // Constants Table
        const constantHeaders = ['编号', '值'];
        const constantKeys = ['index', 'value'];
        tableGrid.appendChild(createTable(data.constants, constantHeaders, constantKeys, '常数表'));
        
        // Activation Record
        const activationHeaders = ['地址', '名称'];
        const activationKeys = ['address', 'name'];
        tableGrid.appendChild(createTable(data.activation_record, activationHeaders, activationKeys, '活动记录映像'));
        
        symbolContainer.appendChild(tableGrid);
    }

    function renderTargetCodeTable(data) {
        const tableBody = document.querySelector("#target-code-table tbody");
        tableBody.innerHTML = ""; // Clear previous results

        if (Array.isArray(data)) {
            data.forEach(item => {
                const row = tableBody.insertRow();
                const lineCell = row.insertCell();
                const codeCell = row.insertCell();
                lineCell.textContent = item.line;
                codeCell.textContent = item.code;
            });
        }
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

    // Load the default example on page load
    fetchAndCompile(editor.getValue(), 'tokens');
}); 