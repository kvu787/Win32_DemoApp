Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# Original 32x32, eight-frame uncompressed silent AVI for SysAnimate32.
function U32([uint32[]]$values) { $stream = [IO.MemoryStream]::new(); $writer = [IO.BinaryWriter]::new($stream); foreach ($v in $values) { $writer.Write($v) }; $writer.Flush(); return ,$stream.ToArray() }
function Chunk([string]$name, [byte[]]$bytes) { $stream = [IO.MemoryStream]::new(); $writer = [IO.BinaryWriter]::new($stream); $writer.Write([Text.Encoding]::ASCII.GetBytes($name)); $writer.Write([uint32]$bytes.Length); $writer.Write($bytes); if ($bytes.Length % 2) { $writer.Write([byte]0) }; $writer.Flush(); return ,$stream.ToArray() }
function ListChunk([string]$type, [byte[]]$bytes) { return ,(Chunk 'LIST' ([byte[]]([Text.Encoding]::ASCII.GetBytes($type) + $bytes))) }
$avih = Chunk 'avih' (U32 @(125000,24576,0,16,8,0,1,3072,32,32,0,0,0,0))
$streamHeader = [byte[]]([Text.Encoding]::ASCII.GetBytes('vidsDIB ') + (U32 @(0,0,0,1,8,0,8,3072,4294967295,0)) + [byte[]]@(0,0,0,0,32,0,32,0))
$format = [byte[]]((U32 @(40,32,32)) + [byte[]]@(1,0,24,0) + (U32 @(0,3072,0,0,0,0)))
$strl = ListChunk 'strl' ([byte[]]((Chunk 'strh' $streamHeader) + (Chunk 'strf' $format)))
$hdrl = ListChunk 'hdrl' ([byte[]]($avih + $strl))
$frames = [IO.MemoryStream]::new()
$index = [IO.MemoryStream]::new()
$indexWriter = [IO.BinaryWriter]::new($index)
for ($f=0; $f -lt 8; $f++) {
    $pixels = [byte[]]::new(3072)
    for ($y=0; $y -lt 32; $y++) { for ($x=0; $x -lt 32; $x++) { $i = ($y*32+$x)*3; $pixels[$i]=[byte](50+$x*5); $pixels[$i+1]=[byte](40+$y*5); $pixels[$i+2]=[byte](($f*28+$x*2)%256) } }
    $indexWriter.Write([Text.Encoding]::ASCII.GetBytes('00db')); $indexWriter.Write([uint32]16); $indexWriter.Write([uint32](4+$frames.Length)); $indexWriter.Write([uint32]3072)
    $chunk = Chunk '00db' $pixels
    $frames.Write($chunk,0,$chunk.Length)
}
$indexWriter.Flush()
$avi = Chunk 'RIFF' ([byte[]]([Text.Encoding]::ASCII.GetBytes('AVI ') + $hdrl + (ListChunk 'movi' $frames.ToArray()) + (Chunk 'idx1' $index.ToArray())))
[IO.File]::WriteAllBytes((Join-Path $PSScriptRoot '..\Source\Demo.avi'),$avi)
