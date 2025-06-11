const express = require('express');
const { exec } = require('child_process');
const fs = require('fs');
const path = require('path');
const cors = require('cors');

const app = express();
const port = 3000;

app.use(cors());
app.use(express.json());
app.use(express.static('public'));

const compilerPath = path.join(__dirname, '..', 'build', 'compiler_app.exe'); // 假设编译器在 build 目录下
const tempCodePath = path.join(__dirname, 'temp_code.tmp');

app.post('/api/compile', (req, res) => {
    const { code, target } = req.body;

    if (!code) {
        return res.status(400).json({ error: 'No code provided.' });
    }

    // 1. 将代码写入临时文件
    fs.writeFile(tempCodePath, code, (err) => {
        if (err) {
            console.error(`Error writing temp file: ${err}`);
            return res.status(500).json({ error: 'Failed to write temporary code file.' });
        }

        // 2. 构建编译器命令
        const command = `"${compilerPath}" --input "${tempCodePath}" --target ${target}`;
        console.log(`Executing command: ${command}`);

        // 3. 执行编译器
        exec(command, (error, stdout, stderr) => {
            if (error) {
                console.error(`Compiler execution error: ${error.message}`);
                // 同时返回 stdout 和 stderr，因为编译器可能在 stdout 中输出部分成功的结果，在 stderr 中输出错误信息
                return res.status(500).json({ error: `Compiler execution failed.`, details: stderr, partial_output: stdout });
            }
            if (stderr) {
                console.warn(`Compiler execution produced warnings: ${stderr}`);
            }

            // 4. 返回结果
            try {
                const jsonOutput = JSON.parse(stdout);
                res.json(jsonOutput);
            } catch (parseError) {
                console.error(`JSON parse error: ${parseError.message}`);
                // 如果 JSON 解析失败，返回原始的 stdout，方便调试
                res.status(500).json({ error: 'Failed to parse compiler output as JSON.', details: stdout });
            }
        });
    });
});

app.listen(port, () => {
    console.log(`Compiler visualization server listening at http://localhost:${port}`);
}); 