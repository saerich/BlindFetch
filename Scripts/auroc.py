from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import StratifiedKFold, cross_val_predict
from sklearn.metrics import roc_auc_score, accuracy_score, balanced_accuracy_score, classification_report, f1_score, recall_score, confusion_matrix
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

CSV_PATH = "Data/MergedExecutions.csv"
N_SPLITS = 5
N_BOOTSTRAPS = 10000
RANDOM_STATE = 42

data = pd.read_csv(CSV_PATH)
data.columns = ["policy","ordering","S","ttfb_ms","ttlb_ms","blocks_sent","perm_id","target_position","Slot0","Slot1","Slot2","Slot3"]
for col in ["ttfb_ms","ttlb_ms","blocks_sent","target_position"]:
    data[col] = pd.to_numeric(data[col])
data["target_position"] = data["target_position"].astype(int)
data["duration"] = data["ttlb_ms"] - data["ttfb_ms"]

TIMING_FEATURES = ["ttfb_ms","ttlb_ms","blocks_sent","duration"]
SLOT_COLS = ["Slot0","Slot1","Slot2","Slot3"]

def slot_to_firmware(row, slot):
    return row[SLOT_COLS[int(slot)]]

def features(sub):
    return pd.get_dummies(sub[TIMING_FEATURES + SLOT_COLS], columns=SLOT_COLS, dtype=int)

def evaluate(y, X):
    cv = StratifiedKFold(n_splits=N_SPLITS, shuffle=True, random_state=RANDOM_STATE)
    model = RandomForestClassifier(n_estimators=100, random_state=RANDOM_STATE)
    pred = cross_val_predict(model, X, y, cv=cv, method="predict")
    prob = cross_val_predict(model, X, y, cv=cv, method="predict_proba")
    labels = np.sort(np.unique(y))
    y_np, pred_np, prob_np = np.asarray(y), np.asarray(pred), np.asarray(prob)
    class_idx = {c: np.flatnonzero(y_np == c) for c in labels}
    rng = np.random.default_rng(RANDOM_STATE)
    boot_acc, boot_f1, boot_auc = [], [], []
    for _ in range(N_BOOTSTRAPS):
        idx = np.concatenate([rng.choice(class_idx[c], len(class_idx[c]), replace=True) for c in labels])
        boot_acc.append(accuracy_score(y_np[idx], pred_np[idx]))
        boot_f1.append(f1_score(y_np[idx], pred_np[idx], average="macro"))
        boot_auc.append(roc_auc_score(y_np[idx], prob_np[idx], multi_class="ovr", average="macro"))
    return {
        "pred": pred, "prob": prob, "labels": labels,
        "accuracy": accuracy_score(y, pred),
        "balanced_accuracy": balanced_accuracy_score(y, pred),
        "macro_f1": f1_score(y, pred, average="macro"),
        "macro_auc": roc_auc_score(y, prob, multi_class="ovr", average="macro"),
        "accuracy_ci": np.percentile(boot_acc,[2.5,97.5]),
        "f1_ci": np.percentile(boot_f1,[2.5,97.5]),
        "auc_ci": np.percentile(boot_auc,[2.5,97.5]),
        "recall": recall_score(y,pred,labels=labels,average=None),
        "cm": confusion_matrix(y,pred,labels=labels),
    }

results=[]
for policy in ["EarlyClose","FixedBudget"]:
    for ordering in ["Ordered","Unordered"]:
        sub=data[(data.policy==policy)&(data.ordering==ordering)].copy()
        if sub.empty: continue
        X=features(sub)

        # Attack 1: infer slot, then map predicted slot through known permutation.
        slot=evaluate(sub["target_position"],X)
        actual_fw=np.array([slot_to_firmware(row,row["target_position"]) for _,row in sub.iterrows()])
        mediated_fw=np.array([slot_to_firmware(row,s) for (_,row),s in zip(sub.iterrows(),slot["pred"])])
        mediated_acc=accuracy_score(actual_fw,mediated_fw)
        mediated_f1=f1_score(actual_fw,mediated_fw,average="macro")

        # Attack 2: infer firmware identity directly.
        direct=evaluate(pd.Series(actual_fw,index=sub.index),X)

        label=policy.replace("EarlyClose","Early Close").replace("FixedBudget","Fixed Budget")+"\n"+ordering.replace("Unordered","Randomized")
        results.append((label,slot,mediated_acc,mediated_f1,direct))

        print("="*90)
        print(f"{policy} + {ordering}")
        print("SLOT-MEDIATED ATTACK")
        print(f"Firmware accuracy: {mediated_acc:.4f}")
        print(f"Firmware macro-F1: {mediated_f1:.4f}")
        print(f"Slot macro-OvR AUROC: {slot['macro_auc']:.4f} [{slot['auc_ci'][0]:.4f}, {slot['auc_ci'][1]:.4f}]")
        print("\nDIRECT FIRMWARE ATTACK")
        print(f"Firmware accuracy: {direct['accuracy']:.4f} [{direct['accuracy_ci'][0]:.4f}, {direct['accuracy_ci'][1]:.4f}]")
        print(f"Firmware macro-F1: {direct['macro_f1']:.4f} [{direct['f1_ci'][0]:.4f}, {direct['f1_ci'][1]:.4f}]")
        print(f"Firmware macro-OvR AUROC: {direct['macro_auc']:.4f} [{direct['auc_ci'][0]:.4f}, {direct['auc_ci'][1]:.4f}]")
        print("\nDirect firmware confusion matrix:")
        print("Labels:",direct["labels"])
        print(direct["cm"])
        print(classification_report(actual_fw,direct["pred"],labels=direct["labels"]))

# One graph containing both attacks.
labels=[r[0] for r in results]
slot_auc=np.array([r[1]["macro_auc"] for r in results])
slot_lo=np.array([r[1]["auc_ci"][0] for r in results])
slot_hi=np.array([r[1]["auc_ci"][1] for r in results])
direct_auc=np.array([r[4]["macro_auc"] for r in results])
direct_lo=np.array([r[4]["auc_ci"][0] for r in results])
direct_hi=np.array([r[4]["auc_ci"][1] for r in results])

x=np.arange(len(labels)); width=.36
fig,ax=plt.subplots(figsize=(9,5))
ax.bar(x-width/2,slot_auc,width,yerr=np.vstack([slot_auc-slot_lo,slot_hi-slot_auc]),capsize=5,label="Slot-mediated attack")
ax.bar(x+width/2,direct_auc,width,yerr=np.vstack([direct_auc-direct_lo,direct_hi-direct_auc]),capsize=5,label="Direct firmware attack")
ax.axhline(.5,linestyle="--",linewidth=1,label="Chance AUROC")
ax.set_xticks(x); ax.set_xticklabels(labels)
ax.set_ylabel("Macro-OvR AUROC"); ax.set_ylim(0,1.1)
ax.set_title("Selection inference by attack type")
ax.legend(); fig.tight_layout()
fig.savefig("selection-inference-auroc.png",dpi=300,bbox_inches="tight")
plt.show()
