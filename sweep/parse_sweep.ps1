# parse_sweep.ps1 - extrait la telemetrie des logs chronicle en 2 CSV (sims + seeds).
# ASCII pur (PS 5.1 lit les .ps1 sans BOM en ANSI) : les mots accentues des logs
# sont matches par \S+ ou des points.
$logs = Get-ChildItem "sweep\logs\*.log"
$simRows = @()
$seedRows = @()
foreach ($f in $logs) {
    $seedName = $f.BaseName -replace "^seed_", ""
    $txt = Get-Content $f.FullName -Raw -Encoding UTF8
    $simHdr = [regex]::Matches($txt, "Sim (\d+) \(graine (\d+)\) . (\d+) empires . (\d+) cit\S+tats . (\d+) continents . (\d+) r\S+gions")
    for ($i = 0; $i -lt $simHdr.Count; $i++) {
        $m = $simHdr[$i]
        $start = $m.Index
        $synIdx = $txt.IndexOf("SYNTH")
        $end = if ($i + 1 -lt $simHdr.Count) { $simHdr[$i + 1].Index } elseif ($synIdx -ge 0) { $synIdx } else { $txt.Length }
        $blk = $txt.Substring($start, $end - $start)
        $row = [ordered]@{
            seed = $seedName; sim = [int]$m.Groups[1].Value; empires = [int]$m.Groups[3].Value
            cites = [int]$m.Groups[4].Value; regions = [int]$m.Groups[6].Value
        }
        foreach ($an in 50, 100, 150, 200) {
            $s = [regex]::Match($blk, "an\s+$an :\s*(\d+) pays \| pop\s+(\d+)k \| arm\S+\s+(\d+) \| colonis\S+\s+(\d+) prov \| transf\. paix\s+(\d+) prov \| 1er empire\s+(\d+) r\S+g \| prosp\s*(-?\d+) stab\s*(-?\d+) \|\s*(\d+) r\S+volt")
            if ($s.Success) {
                $row["pays$an"] = [int]$s.Groups[1].Value; $row["pop$an"] = [int]$s.Groups[2].Value
                $row["top_reg$an"] = [int]$s.Groups[6].Value; $row["stab$an"] = [int]$s.Groups[8].Value
                $row["revolte$an"] = [int]$s.Groups[9].Value
                if ($an -eq 200) { $row["transf200"] = [int]$s.Groups[5].Value; $row["colonisees200"] = [int]$s.Groups[4].Value; $row["armee200"] = [int]$s.Groups[3].Value }
            }
        }
        $fin = [regex]::Match($blk, "27 FIN : ([A-Z ]+) \(an (\d+)\)")
        $row["fin27"] = if ($fin.Success) { $fin.Groups[1].Value.Trim() } else { "-" }
        $row["fin27_an"] = if ($fin.Success) { [int]$fin.Groups[2].Value } else { 0 }
        $bt = [regex]::Match($blk, "batailles : (\d+) livr\S+ . (\d+) j en moy\. . (\d+) d\S+route")
        if ($bt.Success) { $row["batailles"] = [int]$bt.Groups[1].Value }
        $mt = [regex]::Match($blk, "morts : (\d+) au CHOC vs (\d+) en POURSUITE")
        if ($mt.Success) { $row["morts_choc"] = [int]$mt.Groups[1].Value; $row["morts_pursuit"] = [int]$mt.Groups[2].Value }
        $mb = [regex]::Match($blk, "tabolisation : (\d+)/(\d+) empire")
        if ($mb.Success) { $row["metab_n"] = [int]$mb.Groups[1].Value }
        $mb2 = [regex]::Match($blk, "max ([\d.]+)% . \+[\d.]+% recherche")
        if ($mb2.Success) { $row["metab_max"] = [double]$mb2.Groups[1].Value }
        $cb = [regex]::Match($blk, "combos tier-4 : (\d+) empire\(s\) tiennent une fusion d.h\S+ritages . (\d+) combo")
        if ($cb.Success) { $row["combos"] = [int]$cb.Groups[2].Value }
        $ip = [regex]::Match($blk, "IPM final ([\d.]+)")
        if ($ip.Success) { $row["ipm"] = [double]$ip.Groups[1].Value }
        $hg = [regex]::Match($blk, "1er empire (\d+) r\S+g . Stabilit\S+ plancher (-?\d+)(.*)")
        if ($hg.Success) { $row["heg_reg"] = [int]$hg.Groups[1].Value; $row["heg_floor"] = [int]$hg.Groups[2].Value; $row["heg_craque"] = if ($hg.Groups[3].Value -match "CRAQU") { 1 } else { 0 } }
        $mer = [regex]::Match($blk, "mer : (\d+) coque\(s\) b\S+tie")
        if ($mer.Success) { $row["coques"] = [int]$mer.Groups[1].Value }
        $rm = [regex]::Match($blk, "(\d+) route\(s\) maritime\(s\) . (\d+) colonie\(s\) outre-mer")
        if ($rm.Success) { $row["routes_mer"] = [int]$rm.Groups[1].Value; $row["colonies_mer"] = [int]$rm.Groups[2].Value }
        $simRows += [pscustomobject]$row
    }
    $synIdx2 = $txt.IndexOf("SYNTH")
    if ($synIdx2 -lt 0) { $synIdx2 = 0 }
    $syn = $txt.Substring($synIdx2)
    $sr = [ordered]@{ seed = $seedName }
    $sat = [regex]::Match($syn, "Laborer (\d+)% . Bourgeois (\d+)% . \S+lite (\d+)%")
    if ($sat.Success) { $sr["sat_L"] = [int]$sat.Groups[1].Value; $sr["sat_B"] = [int]$sat.Groups[2].Value; $sr["sat_E"] = [int]$sat.Groups[3].Value }
    $g = [regex]::Match($syn, "guerres d\S+clench\S+es \(total\) . (\d+)"); if ($g.Success) { $sr["guerres"] = [int]$g.Groups[1].Value }
    $al = [regex]::Match($syn, "alliances actives \(fin de sim\) (\d+)"); if ($al.Success) { $sr["alliances"] = [int]$al.Groups[1].Value }
    $rg = [regex]::Match($syn, "religion \.+ ([\d.]+) foi\(s\) fond\S+e\(s\)/sim . ([\d.]+) schisme"); if ($rg.Success) { $sr["religion_sim"] = [double]$rg.Groups[1].Value; $sr["schismes_sim"] = [double]$rg.Groups[2].Value }
    $hm = [regex]::Match($syn, "MORTEL \(A5\) \.+ (\d+)/(\d+) sims"); if ($hm.Success) { $sr["heg_mortel"] = "$($hm.Groups[1].Value)/$($hm.Groups[2].Value)" }
    $ab = [regex]::Match($syn, "pays absorb\S+s \(morts\) \.+ (\d+)"); if ($ab.Success) { $sr["absorbes"] = [int]$ab.Groups[1].Value }
    $em = [regex]::Match($syn, "pays \S+merg\S+s \(s\S+cession\) \.+ (\d+)"); if ($em.Success) { $sr["secessions"] = [int]$em.Groups[1].Value }
    $wd = [regex]::Match($syn, "hameaux libres \(WILD\) \.+ ([\d.]+) sem\S+s/sim . (\d+) ralli\S+s"); if ($wd.Success) { $sr["wild_rallies"] = [int]$wd.Groups[2].Value }
    $ax = [regex]::Match($syn, ". (\d+) annexion\(s\) par digestion"); if ($ax.Success) { $sr["annexions"] = [int]$ax.Groups[1].Value }
    $hb = [regex]::Match($syn, "hubs des cit\S+tats \.+ (\d+)%"); if ($hb.Success) { $sr["hub_pct"] = [int]$hb.Groups[1].Value }
    $sl = [regex]::Match($syn, "soul\S+vements incarn\S+s \.+ (\d+) allum\S+s . (\d+) s\S+cession\(s\) . (\d+) coup"); if ($sl.Success) { $sr["soulev"] = [int]$sl.Groups[1].Value; $sr["coups"] = [int]$sl.Groups[3].Value }
    $rc = [regex]::Match($txt, "RC=(\d+) DUREE=(\d+)s"); if ($rc.Success) { $sr["rc"] = [int]$rc.Groups[1].Value; $sr["duree_s"] = [int]$rc.Groups[2].Value }
    $seedRows += [pscustomobject]$sr
}
$simRows | Export-Csv "sweep\sims.csv" -NoTypeInformation -Encoding UTF8
$seedRows | Export-Csv "sweep\seeds.csv" -NoTypeInformation -Encoding UTF8
Write-Host ("sims: " + $simRows.Count + "  seeds: " + $seedRows.Count)
