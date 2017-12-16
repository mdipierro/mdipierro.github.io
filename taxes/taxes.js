"use strict";

var data = {
     married: true,
     gross: 120000,
     self_amount: 20000,
     dependants: 2,
     state_tax_rate: 5,
     re_taxes: 6000,
     delta: 0
};
var rates = {
    "old": {
        "1":[[9325, 10],
             [37950, 15],
             [91900, 25],
             [191650, 28],
             [416700, 33],
             [418400, 35],
             [null, 39.6]],
        "2":[[18650, 10],
             [75900, 15],
             [153100, 25],
             [233350, 28],
             [416700, 33],
             [470700, 35],
             [null, 39.6]]
    },
    "new": {
        "1":[[9525, 10],
             [38700, 12],
             [82500, 22],
             [157500, 24],
             [200000, 32],
             [500000, 35],
             [null, 37]],
        "2":[[19050, 10],
             [77400, 12],
             [165000, 22],
             [315000, 23],
             [400000, 32],
             [600000, 35],
             [null, 37]]
    }
};
var compute_tax = function(amt, rate) {
    var a = 0, tax = 0;
    if(amt<0) return 0;
    rate.map(function(item) {
            if(a < amt) {
                if(item[0])
                    tax += 0.01 * item[1] * (Math.min(amt, item[0]) - a);
                else
                    tax += 0.01 * item[1] * (amt - a);
            }
            a = item[0];
        });
    return tax;
};
var methods = {
    compute: function() {
        var amt, new_tax, old_tax;
        var gross = parseInt(vue.gross || 0);
        var state_tax_rate = parseFloat(vue.state_tax_rate || 0);
        var re_taxes = parseInt(vue.re_taxes || 0);
        var self_amount = parseInt(vue.self_amount || 0);
        var dependants = parseFloat(vue.dependants || 0);
        if(!vue.married) {
            amt = gross - (6300 + 
                           gross * state_tax_rate/100 +
                           re_taxes + 
                           dependants * 4050 * ((gross<=155650)?1:0));            
            old_tax = compute_tax(amt, rates["old"]['1']);
            amt = gross - (12000 + 
                           0.20 * Math.min(self_amount, Math.max(gross-315000,0)) +
                           dependants * 4050 * ((gross<=155650)?1:0));            
            new_tax = compute_tax(amt,rates["new"]['1']);            
        } else {
            amt = gross - (12600 + 
                           gross * state_tax_rate/100 +
                           re_taxes + 
                           dependants * 4050 * ((gross<=155650)?1:0));            
            old_tax = compute_tax(amt, rates["old"]['2']);
            amt = gross - (24000 +
                           0.20*Math.min(self_amount, Math.max(gross-315000,0)) +
                           dependants * 4050 * ((gross<=155650)?1:0));            
            new_tax = compute_tax(amt,rates["new"]['2']);
        }
        vue.delta = parseInt(new_tax - old_tax);        
    }
};

var vue = new Vue({data: data, methods: methods, el:'#target'});
vue.compute();